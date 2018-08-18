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
    the_world->start_component(sm);

    sm->run();

    delete the_world;
    return 0;
}

void World::gc()
{
    auto it = _entities.begin();
    auto end_it = _entities.end();
    while (it != end_it)
    {
        auto &entity = *it;

        if (!entity->is_removed())
        {
            ++it;
            continue;
        }

        entity->clear();
        _entities.erase(it++);
    }
}

std::shared_ptr<Entity> World::create_entity()
{
    std::shared_ptr<Entity> entity(new Entity());
    _entities.push_back(entity);
    return entity;
}

void World::start_component(const std::shared_ptr<Component> &component)
{
    auto entity = create_entity();
    entity->add_component(component);
}
