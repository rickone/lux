#pragma once

#include <atomic>
#include "lua_port.h"
#include "queue.h"

namespace lux {

class Channel : public Object {
public:
    Channel() = default;
    virtual ~Channel() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<Channel> create();

    int lua_push_(lua_State* L);
    int lua_pop_(lua_State* L);

private:
    lux::Queue _queue;
};

} // lux
