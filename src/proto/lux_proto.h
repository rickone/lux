#pragma once

#include <unordered_set>
#include <memory>
#include "lux_proto_def.h"
#include "lua_port.h"

namespace lux {

class Proto;

struct Protable
{
    virtual ~Protable() {}
    virtual void pack(Proto *proto) = 0;
    virtual void unpack(Proto *proto) = 0;
};

class Proto : public Object
{
public:
    Proto() = default;
    Proto(const char *data, size_t len);
    explicit Proto(const std::string &str);
    Proto(const Proto &other);
    virtual ~Proto() = default;
    Proto & operator =(const Proto &other);

    static void new_class(lua_State *L);
    static std::shared_ptr<Proto> create();

    void clear();
    std::string dump();
    int lua_pack(lua_State *L);
    int lua_packlist(lua_State *L);
    int lua_unpack(lua_State *L);
    void pack_lua_object(lua_State *L, int index);
    int unpack_lua_object(lua_State *L);

    template<typename T>
    void pack(T t)
    {
        pack_impl(
            t,
            typename std::is_base_of<Protable, typename std::decay<typename std::remove_pointer<T>::type>::type>::type()
        );
    }

    template<typename T, typename C>
    void pack_impl(T t, C &&)
    {
        LuxProtoDef<T>::pack(_str, t);
    }

    template<typename T>
    void pack_impl(T t, std::true_type &&)
    {
        static_assert(std::is_pointer<T>::value, "should be a pointer");
        t->pack(this);
    }

    template<typename T, typename...A>
    void pack_args(T t, A...args)
    {
        pack(t);
        pack_args(args...);
    }

    void pack_args()
    {
    }

    template<typename T>
    T unpack()
    {
        return unpack_impl<T>(
            typename std::is_pointer<T>::type(),
            typename std::is_base_of<Protable, typename std::decay<typename std::remove_pointer<T>::type>::type>::type()
        );
    }

    template<typename T, typename C1, typename C2>
    T unpack_impl(C1 &&, C2 &&)
    {
        size_t used_len = 0;
        T result = LuxProtoDef<T>::unpack(_str, _pos, &used_len);
        _pos += used_len;
        return result;
    }

    template<typename T>
    T unpack_impl(std::false_type &&, std::true_type &&)
    {
        auto object = std::make_shared<T>();
        _proto_objs.insert(object);
        object->unpack(this);
        return *object;
    }

    template<typename T>
    T unpack_impl(std::true_type &&, std::true_type &&)
    {
        auto object = std::make_shared<typename std::decay<typename std::remove_pointer<T>::type>::type>();
        _proto_objs.insert(object);
        object->unpack(this);
        return object.get();
    }

    template<typename...A>
    void invoke(const std::function<void (A...args)> &func)
    {
        func(unpack<typename std::decay<A>::type>()...);
    }

    template<typename...A>
    void invoke(void (*func)(A...args))
    {
        func(unpack<typename std::decay<A>::type>()...);
    }

    template<typename R, typename...A>
    R call(const std::function<R (A...args)> &func)
    {
        return func(unpack<typename std::decay<A>::type>()...);
    }

    template<typename R, typename...A>
    R call(R (*func)(A...args))
    {
        return func(unpack<typename std::decay<A>::type>()...);
    }

    const std::string & str() const { return _str; }
    void set_str(const std::string &str) { _str = str; }

    size_t pos() const { return _pos; }
    void set_pos(size_t pos) { _pos = pos; }

private:
    std::string _str;
    size_t _pos = 0;
    std::unordered_set< std::shared_ptr<Protable> > _proto_objs;
};

} // lux
