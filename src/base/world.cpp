#include "world.h"
#include "system_manager.h"

World *world = nullptr;

World::World()
{
    world = this;
}

World::~World()
{
    world = nullptr;
}

int World::launch(int argc, char *argv[])
{
    World *the_world = new World();

    std::shared_ptr<SystemManager> sm(new SystemManager(argc, argv));
    the_world->create_object_with_component(sm);

    sm->run();

    delete the_world;
    return 0;
}

void World::gc()
{
    for (auto it = _objects.begin(); it != _objects.end(); )
    {
        auto &object = *it;

        if (!object->is_removed())
        {
            ++it;
            continue;
        }

        _objects.erase(it++);
    }
}

std::shared_ptr<Entity> World::create_object()
{
    std::shared_ptr<Entity> object(new Entity());
    _objects.push_back(object);
    return object;
}

std::shared_ptr<Entity> World::create_object_with_component(const std::shared_ptr<Component> &component)
{
    auto object = create_object();
    object->add_component(component);
    return object;
}
