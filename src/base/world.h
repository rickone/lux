#pragma once

#include <list>
#include "entity.h"

class World final
{
public:
    World();
    ~World();

    static int launch(int argc, char *argv[]);

    void gc();
    std::shared_ptr<Entity> create_entity();
    void start_component(const std::shared_ptr<Component> &component);

private:
    std::list< std::shared_ptr<Entity> > _entities;
};

extern World *world;
