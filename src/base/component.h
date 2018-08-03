#pragma once

#include <functional>
#include <unordered_map>
#include <list>
#include "lua_port.h"
#include "timer.h"

enum MessageType
{
    kMsg_SocketError,
    kMsg_SocketClose,
    kMsg_SocketAccept,
    kMsg_SocketRecv,
    kMsg_SocketSend,
    kMsg_KcpRecv,
    kMsg_KcpSend,
    kMsg_PackageRecv,
    kMsg_PackageSend,
};

typedef std::function<void (LuaObject *)> MessageFunction;
class Entity;

class Component : public LuaObject
{
public:
    explicit Component(const char *name = "noname");
    virtual ~Component() = default;

    static void new_class(lua_State *L);

    virtual void start(LuaObject *init_object);
    virtual void stop() noexcept;

    void subscribe(int msg_type, const MessageFunction &func);
    void unsubscribe(int msg_type);
    void on_msg(int msg_type, LuaObject *msg_object);
    void publish(int msg_type, LuaObject *msg_object);
    void remove();
    void remove_entity();

    bool is_removed() { return _removed; }

    std::string name() { return _name; }
    void set_name(const std::string &name) { _name = name; }

    Entity* entity() { return _entity; }
    void set_entity(Entity *entity) { _entity = entity; }

    template<typename T>
    void set_timer(T *object, const std::function<void (T *, Timer *)> &func, int interval, int counter = -1);

    template<typename T>
    void set_timer(T *object, void (T::*func)(Timer *), int interval, int counter = -1);

    template<typename T>
    void subscribe(int msg_type, void (T::*func)(LuaObject *));

protected:
    std::string _name;
    Entity* _entity;
    bool _removed;
    std::unordered_map<int, MessageFunction> _msg_map;
};

template<typename T>
void Component::set_timer(T *object, const std::function<void (T *, Timer *)> &func, int interval, int counter)
{
    static_assert(std::is_base_of<Component, T>::value, "Component::set_timer failed, T must based on Component");

    auto timer = Timer::create(interval, counter);
    timer->set_callback(object, func);
}

template<typename T>
void Component::set_timer(T *object, void (T::*func)(Timer *), int interval, int counter)
{
    std::function<void (T *, Timer *)> mfn = std::mem_fn(func);
    set_timer(object, mfn, interval, counter);
}

template<typename T>
void Component::subscribe(int msg_type, void (T::*func)(LuaObject *))
{
    static_assert(std::is_base_of<Component, T>::value, "Component::subscribe failed, need a Component-based mem-func");

    _msg_map[msg_type] = std::bind(func, (T *)this, std::placeholders::_1);
}

struct LuaMessageObject : public LuaObject
{
    int arg_begin;
    int arg_end;

    virtual int lua_push_self(lua_State *L) override
    {
        for (int i = arg_begin; i <= arg_end; ++i)
        {
            lua_pushvalue(L, i);
        }
        return arg_end - arg_begin + 1;
    }
};
