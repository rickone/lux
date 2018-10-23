#pragma once

#include <memory> // shared_ptr
#include <string>
#include <vector>
#include "redis_proto_def.h"
#include "redis_object.h"
#include "lua_port.h"

namespace lux {

class RedisProto : public Object
{
public:
    RedisProto() = default;
    virtual ~RedisProto() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<RedisProto> create();
  
    void clear();
    void pack(std::nullptr_t ptr);
    void pack(const RedisObject &obj);
    void pack(const std::string &value);
    void pack_bulk_string(const char *data, size_t len);

    template<typename T>
    void pack(T t)
    {
        static_assert(std::is_integral<T>::value, "RedisProto.pack need a integral type");
        redis_pack_integer(_str, t);
    }

    std::shared_ptr<RedisObject> unpack();
    int lua_pack(lua_State *L);
    int lua_unpack(lua_State *L);
    void pack_lua_object(lua_State *L, int index);
    int unpack_lua_object(lua_State *L);

    const std::string & str() const { return _str; }
    void set_str(const std::string &str) { _str = str; }

    size_t pos() const { return _pos; }
    void set_pos(size_t pos) { _pos = pos; }

private:
    std::string _str;
    size_t _pos = 0;
};

} // lux
