#pragma once

#include "lux_proto_def.h"
#include "lua_port.h"

class LuxProto : public LuaObject
{
public:
    LuxProto() = default;
    virtual ~LuxProto() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<LuxProto> create();

    void clear();
    std::string dump();
    int lua_pack(lua_State *L);
    int lua_packlist(lua_State *L);
    int lua_unpack(lua_State *L);

    template<typename T>
    void pack(T t)
    {
        LuxProtoDef<T>::pack(_str, t);
    }

    template<typename T>
    T unpack()
    {
        size_t used_len = 0;
        T result = LuxProtoDef<T>::unpack(_str, _pos, &used_len);
        _pos += used_len;
        return result;
    }

    template<typename...A>
    void invoke(const std::function<void (A...args)> &func)
    {
        func(unpack<typename lux_proto_unpack_type<A>::type>()...);
    }

    template<typename...A>
    void invoke(void (*func)(A...args))
    {
        func(unpack<typename lux_proto_unpack_type<A>::type>()...);
    }

    template<typename R, typename...A>
    R call(const std::function<R (A...args)> &func)
    {
        return func(unpack<typename lux_proto_unpack_type<A>::type>()...);
    }

    template<typename R, typename...A>
    R call(R (*func)(A...args))
    {
        return func(unpack<typename lux_proto_unpack_type<A>::type>()...);
    }

    size_t pos() const { return _pos; }
    void set_pos(size_t pos) { _pos = pos; }

    const std::string & str() const { return _str; }
    void set_str(const std::string &str) { _str = str; }

private:
    void lua_pack_one(lua_State *L, int index);
    int lua_unpack_one(lua_State *L);

private:
    std::string _str;
    size_t _pos = 0;
};

/*
// std::list
template<typename T>
void LuxProto::pack(const std::list<T> &value)
{
    _str.push_back(LUX_HEADER_LIST);
    varint_pack(_str, value.size());
    for (auto &item : value)
    {
        pack(item);
    }
}

template<typename T>
std::list<T> LuxProto::unpack()
{
    uint8_t header = (uint8_t)_str.at(_pos);
    if (header != LUX_HEADER_LIST)
        throw_error(std::runtime_error, "header=0x%02X", header);

    size_t var_len = 0;
    size_t size = varint_unpack(_str, _pos + 1, &var_len);
    _pos += 1 + var_len;

    std::list<T> value;
    for (size_t i = 0; i < size; ++i)
    {
        T item = unpack<T>();
        value.push_back(item);
    }
    return value;
}

// std::vector
template<typename T>
void LuxProto::pack(const std::vector<T> &value)
{
    _str.push_back(LUX_HEADER_LIST);
    varint_pack(_str, value.size());
    for (auto &item : value)
    {
        pack(item);
    }
}

template<typename T>
std::vector<T> LuxProto::unpack()
{
    uint8_t header = (uint8_t)_str.at(_pos);
    if (header != LUX_HEADER_LIST)
        throw_error(std::runtime_error, "header=0x%02X", header);

    size_t var_len = 0;
    size_t size = varint_unpack(_str, _pos + 1, &var_len);
    _pos += 1 + var_len;

    std::vector<T> value;
    for (size_t i = 0; i < size; ++i)
    {
        T item = unpack<T>();
        value.push_back(item);
    }
    return value;
}

// std::map
template<typename K, typename V>
void LuxProto::pack(const std::map<K, V> &value)
{
    _str.push_back(LUX_HEADER_DICT);
    varint_pack(_str, value.size());
    for (auto &pair : value)
    {
        pack(pair.first);
        pack(pair.second);
    }
}

template<typename K, typename V>
std::map<K, V> LuxProto::unpack()
{
    uint8_t header = (uint8_t)_str.at(_pos);
    if (header != LUX_HEADER_DICT)
        throw_error(std::runtime_error, "header=0x%02X", header);

    size_t var_len = 0;
    size_t size = varint_unpack(_str, _pos + 1, &var_len);
    _pos += 1 + var_len;

    std::map<K, V> dict;
    for (size_t i = 0; i < size; ++i)
    {
        K key = unpack<K>();
        V value = unpack<V>();
        dict.push_back(std::make_pair(key, value));
    }
    return dict;
}
*/
