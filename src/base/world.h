#pragma once

#include <list>
#include "entity.h"

class World final
{
public:
    World();
    ~World();

    static World * inst();
    static int launch(int argc, char *argv[]);

    void clear();
    void gc();
    std::shared_ptr<Entity> create_entity();
    void start_component(const std::shared_ptr<Component> &component);
    void start_lua_component(lua_State *L);

private:
    std::list< std::shared_ptr<Entity> > _entities;
};
