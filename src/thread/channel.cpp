#include "channel.h"
#include <cstdlib> // malloc
#include "lux_proto.h"

using namespace lux;

void Channel::new_class(lua_State *L) {
    lua_new_class(L, Channel);

    lua_newtable(L); {
        lua_set_method(L, "push", lua_push_);
        lua_set_method(L, "pop", lua_pop_);
    } lua_setfield(L, -2, "__method");

    lua_lib(L, "lux_core"); {
        lua_set_method(L, "create_channel", create);
    } lua_pop(L, 1);
}

std::shared_ptr<Channel> Channel::create() {
    return std::make_shared<Channel>();
}

int Channel::lua_push_(lua_State* L) {
    Proto pt;
    pt.lua_pack(L);

    _queue.push(pt.str().data(), pt.str().size());
    return 0;
}

int Channel::lua_pop_(lua_State* L) {
    auto node = _queue.pop();
    if (node == nullptr)
        return 0;

    Proto pt(node->data, node->len);
    return pt.lua_unpack(L);
}
