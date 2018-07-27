#include "component.h"
#include "entity.h"
#include "log.h"

Component::Component(const char *name) : _name(name), _entity(), _removed(), _msg_map()
{
}

void Component::new_class(lua_State *L)
{
    lua_new_class(L, Component);

    lua_newtable(L);
    {
        lua_method(L, unsubscribe);
        lua_method(L, remove);
    }
    lua_setfield(L, -2, "__method");

    lua_newtable(L);
    {
        lua_property(L, name);
        lua_property_readonly(L, entity);
    }
    lua_setfield(L, -2, "__property");
}

void Component::start(LuaObject *init_object)
{
}

void Component::stop() noexcept
{
}

void Component::subscribe(int msg_type, const MessageFunction &func)
{
    _msg_map[msg_type] = func;
}

void Component::unsubscribe(int msg_type)
{
    _msg_map.erase(msg_type);
}

void Component::on_msg(int msg_type, LuaObject *msg_object)
{
    auto it = _msg_map.find(msg_type);
    if (it == _msg_map.end())
        return;

    it->second(msg_object);
}

void Component::publish_msg(int msg_type, LuaObject *msg_object)
{
    if (!_entity)
    {
        log_error("Component('%s') can't publish: entity is null", typeid(*this).name());
        return;
    }

    _entity->publish_msg(msg_type, msg_object);
}

void Component::remove()
{
    stop();
    _removed = true;
}
