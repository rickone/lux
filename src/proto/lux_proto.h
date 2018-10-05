#pragma once

#include <cstddef> // size_t
#include <string>
#include <list>
#include <vector>
#include <map>
#include "lux_proto_def.h"
#include "error.h"
#include <cstdio>

class LuxProto
{
public:
    LuxProto() = default;

    void clear();

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

    /*
    template<typename T>
    void pack(const std::list<T> &value);

    template<typename T>
    void pack(const std::vector<T> &value);

    template<typename T>
    std::vector<T> unpack();

    template<typename K, typename V>
    void pack(const std::map<K, V> &value);

    template<typename K, typename V>
    std::map<K, V> unpack();
    */

    template<typename...A>
    void invoke(const std::function<void(A...args)> &func)
    {
        func(unpack<typename lux_proto_unpack_type<A>::type>()...);
    }

    template<typename...A>
    void invoke(void (*func)(A...args))
    {
        func(unpack<typename lux_proto_unpack_type<A>::type>()...);
    }

    size_t pos() const { return _pos; }
    void set_pos(size_t pos) { _pos = pos; }

    std::string & data() { return _str; }
    void set_data(const std::string &str) { _str = str; }

    std::string dump()
    {
        std::string result = "result=(";
        for (char c : _str)
        {
            static char temp[32];
            sprintf(temp, "%02x,", (unsigned char)c);
            result += temp;
        }
        result += ")";
        return result;
    }
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
