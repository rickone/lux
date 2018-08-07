#include "component.h"
#include "entity.h"

void Component::new_class(lua_State *L)
{
    lua_new_class(L, Component);

    lua_newtable(L);
    {
        lua_property_readonly(L, entity);
        lua_property_readonly(L, name);
    }
    lua_setfield(L, -2, "__property");
}

void Component::start()
{
}

void Component::stop() noexcept
{
}

const char * Component::name() const
{
    return "noname";
}
