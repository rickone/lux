#pragma once

#include "lua_port.h"
#include "queue.h"

namespace lux {

class Routine : public Object {
public:
    enum Status {
        kStatus_Idle,
        kStatus_Running,
        kStatus_Finished,
    };

    Routine();
    virtual ~Routine();

    static void new_class(lua_State *L);
    static int create(lua_State* L);

    bool init(lua_State* L);
    int activate();
    void resume_data(const char* data, size_t len);

    void push(const char* data, size_t len) { _queue.push(data, len); }
    int status() const { return _status; }

private:
    lua_State* _co = nullptr;
    int _co_ref = LUA_NOREF;
    int _func_ref = LUA_NOREF;

    int _status;
    Queue _queue;
};

} // lux
