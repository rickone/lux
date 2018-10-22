#include "object_manager.h"
#include "log.h"

using namespace lux;

ObjectManager * ObjectManager::inst()
{
    static ObjectManager s_inst;
    return &s_inst;
}

void ObjectManager::add_object(const std::shared_ptr<Object> &object)
{
    int id = ++_last_id;
    object->set_id(id);
    _objects.insert(std::make_pair(id, object));
}

std::shared_ptr<Object> ObjectManager::get_object(int id)
{
    auto it = _objects.find(id);
    if (it == _objects.end())
        return nullptr;
    
    return it->second;
}

void ObjectManager::gc()
{
    auto it = _objects.begin();
    auto it_end = _objects.end();
    while (it != it_end)
    {
        auto &object = it->second;
        if (object->is_valid())
        {
            ++it;
            continue;
        }

        log_debug("object gc: %p", object.get());
        _objects.erase(it++);
    }
}
