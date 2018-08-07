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
    std::shared_ptr<Entity> create_object();
    std::shared_ptr<Entity> create_object_with_component(const std::shared_ptr<Component> &component);

private:
    std::list< std::shared_ptr<Entity> > _objects;
};

extern World *world;
