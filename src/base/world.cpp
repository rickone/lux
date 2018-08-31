#include "world.h"
#include "lux_core.h"

static World *s_inst = nullptr;

World::World()
{
    s_inst = this;
}

World::~World()
{
    for (auto &entity : _entities)
        entity->remove();

    s_inst = nullptr;
}

World * World::inst()
{
    return s_inst;
}

int World::launch(int argc, char *argv[])
{
    auto the_world = std::make_shared<World>();

    std::shared_ptr<LuxCore> sm(new LuxCore(argc, argv));
    the_world->start_component(sm);

    sm->run();
    return 0;
}

void World::clear()
{
    _entities.clear();
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

void World::start_lua_component(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);

    std::shared_ptr<Component> component(new Component());
    component->lua_init(L);

    start_component(component);
}
