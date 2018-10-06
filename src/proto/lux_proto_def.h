#pragma once

#include <cstddef> // size_t
#include <string>
#include <list>
#include <vector>
#include <map>
#include "variant_int.h"
#include "error.h"

/* Header Byte Code
*
* Varient Int:
* 0xxx,xxxx -- 1B
* 10xx,xxxx -> [1xxx,xxxx] -> [0xxx,xxxx] -- nB littile-endian
* 1100,0000 -- 1B type
*
* 0xC0 - nil/nullptr +[0]
* 0xC1 - false +[0]
* 0xC2 - true +[0]
* 0xC3 - number/float +[4]
* 0xC4 - number/double +[8]
* 0xC5 - string/const char * +(len:varint) +[len]
* 0xC6 - list/vector +(n:varint) +(value) x n
* 0xC7 - dict/map +(n:varint) +(key,value) x n
* 0xC8 - object +(len:varint) +[len]
*/

#define LUX_HEADER_VARINT   (0xFF)
#define LUX_HEADER_NULL     (0xC0)
#define LUX_HEADER_FALSE    (0xC1)
#define LUX_HEADER_TRUE     (0xC2)
#define LUX_HEADER_FLOAT    (0xC3)
#define LUX_HEADER_DOUBLE   (0xC4)
#define LUX_HEADER_STRING   (0xC5)
#define LUX_HEADER_LIST     (0xC6)
#define LUX_HEADER_DICT     (0xC7)
#define LUX_HEADER_OBJECT   (0xC8)

template<typename T>
struct lux_proto_unpack_type
{
    typedef T type;
};

template<>
struct lux_proto_unpack_type<const std::string &>
{
    typedef std::string type;
};

template<typename T>
struct lux_proto_unpack_type<const std::list<T> &>
{
    typedef std::list<T> type;
};

template<typename T>
struct lux_proto_unpack_type<const std::vector<T> &>
{
    typedef std::vector<T> type;
};

template<typename K, typename V>
struct lux_proto_unpack_type<const std::map<K, V> &>
{
    typedef std::map<K, V> type;
};

template<typename T>
struct LuxProtoDef;

// nullptr
template<>
struct LuxProtoDef<std::nullptr_t>
{
    static void pack(std::string &str, std::nullptr_t ptr)
    {
        str.push_back(LUX_HEADER_NULL);
    }

    static std::nullptr_t unpack(const std::string &str, size_t pos, size_t *used_len)
    {
        uint8_t header = (uint8_t)str.at(pos);
        if (header != LUX_HEADER_NULL)
            throw_error(std::runtime_error, "header=0x%02X", header);

        *used_len = 1;
        return nullptr;
    }
};

// int types
#define lux_proto_int_def(c_type, width) \
    template<> \
    struct LuxProtoDef<c_type> \
    { \
        static void pack(std::string &str, c_type value) \
        { \
            varint_pack(str, zigzag_encode##width(value)); \
        } \
        static c_type unpack(const std::string &str, size_t pos, size_t *used_len) \
        { \
            return zigzag_decode##width(varint_unpack(str, pos, used_len)); \
        } \
    }; \
    template<> \
    struct LuxProtoDef<unsigned c_type> \
    { \
        static void pack(std::string &str, unsigned c_type value) \
        { \
            varint_pack(str, value); \
        } \
        static unsigned c_type unpack(const std::string &str, size_t pos, size_t *used_len) \
        { \
            return varint_unpack(str, pos, used_len); \
        } \
    }

lux_proto_int_def(char, 32);
lux_proto_int_def(short, 32);
lux_proto_int_def(int, 32);
lux_proto_int_def(long, 64);
lux_proto_int_def(long long, 64);

// bool
template<>
struct LuxProtoDef<bool>
{
    static void pack(std::string &str, bool value)
    {
        str.push_back(value ? LUX_HEADER_TRUE : LUX_HEADER_FALSE);
    }

    static bool unpack(const std::string &str, size_t pos, size_t *used_len)
    {
        uint8_t header = (uint8_t)str.at(pos);
        *used_len = 1;

        if (header == LUX_HEADER_FALSE)
            return false;

        if (header == LUX_HEADER_TRUE)
            return true;

        throw_error(std::runtime_error, "header=0x%02X", header);
    }
};

// float
template<>
struct LuxProtoDef<float>
{
    static void pack(std::string &str, float value)
    {
        str.push_back(LUX_HEADER_FLOAT);
        str.append((const char *)&value, sizeof(value));
    }

    static float unpack(const std::string &str, size_t pos, size_t *used_len)
    {
        uint8_t header = (uint8_t)str.at(pos);
        if (header != LUX_HEADER_FLOAT)
            throw_error(std::runtime_error, "header=0x%02X", header);

        float value = 0;
        str.copy((char *)&value, sizeof(value), pos + 1);
        *used_len = 1 + sizeof(float);
        return value;
    }
};

// double
template<>
struct LuxProtoDef<double>
{
    static void pack(std::string &str, double value)
    {
        str.push_back(LUX_HEADER_DOUBLE);
        str.append((const char *)&value, sizeof(value));
    }

    static double unpack(const std::string &str, size_t pos, size_t *used_len)
    {
        uint8_t header = (uint8_t)str.at(pos);
        if (header != LUX_HEADER_DOUBLE)
            throw_error(std::runtime_error, "header=0x%02X", header);

        double value = 0;
        str.copy((char *)&value, sizeof(value), pos + 1);
        *used_len = 1 + sizeof(double);
        return value;
    }
};

// basic string
inline void lux_pack_string(std::string &str, const char *data, size_t len)
{
    str.push_back(LUX_HEADER_STRING);
    varint_pack(str, len);
    str.append(data, len);
    str.push_back(0);
}

inline const char * lux_unpack_string(const std::string &str, size_t pos, size_t *str_len, size_t *used_len)
{
    uint8_t header = (uint8_t)str.at(pos);
    if (header != LUX_HEADER_STRING)
        throw_error(std::runtime_error, "header=0x%02X", header);

    size_t str_len_len = 0;
    *str_len = varint_unpack(str, pos + 1, &str_len_len);

    const char *result = str.c_str() + pos + 1 + str_len_len;
    *used_len = 1 + str_len_len + *str_len + 1;

    return result;
}

// const char *
template<>
struct LuxProtoDef<const char *>
{
    static void pack(std::string &str, const char *value)
    {
        size_t len = strlen(value);
        lux_pack_string(str, value, len);
    }

    static const char * unpack(const std::string &str, size_t pos, size_t *used_len)
    {
        size_t len = 0;
        return lux_unpack_string(str, pos, &len, used_len);
    }
};

// std::string
template<>
struct LuxProtoDef<std::string>
{
    static void pack(std::string &str, const std::string &value)
    {
        lux_pack_string(str, value.data(), value.size());
    }

    static std::string unpack(const std::string &str, size_t pos, size_t *used_len)
    {
        size_t len = 0;
        const char *data = lux_unpack_string(str, pos, &len, used_len);
        return std::string(data, len);
    }
};

// std::list
template<typename T>
struct LuxProtoDef< std::list<T> >
{
    static void pack(std::string &str, const std::list<T> &lst)
    {
        str.push_back(LUX_HEADER_LIST);
        varint_pack(str, lst.size());
        for (const T &t : lst)
            LuxProtoDef<T>::pack(str, t);
    }

    static std::list<T> unpack(const std::string &str, size_t pos, size_t *used_len)
    {
        uint8_t header = (uint8_t)str.at(pos);
        if (header != LUX_HEADER_LIST)
            throw_error(std::runtime_error, "header=0x%02X", header);

        size_t lst_len_len = 0;
        size_t lst_len = varint_unpack(str, pos + 1, &lst_len_len);

        std::list<T> result;
        size_t offset = pos + 1 + lst_len_len;
        for (size_t i = 0; i < lst_len; ++i)
        {
            size_t len = 0;
            T item = LuxProtoDef<T>::unpack(str, offset, &len);
            offset += len;
            result.push_back(item);
        }
        *used_len = offset - pos;
        return result;
    }
};

// std::vector
template<typename T>
struct LuxProtoDef< std::vector<T> >
{
    static void pack(std::string &str, const std::vector<T> &lst)
    {
        str.push_back(LUX_HEADER_LIST);
        varint_pack(str, lst.size());
        for (const T &t : lst)
            LuxProtoDef<T>::pack(str, t);
    }

    static std::vector<T> unpack(const std::string &str, size_t pos, size_t *used_len)
    {
        uint8_t header = (uint8_t)str.at(pos);
        if (header != LUX_HEADER_LIST)
            throw_error(std::runtime_error, "header=0x%02X", header);

        size_t lst_len_len = 0;
        size_t lst_len = varint_unpack(str, pos + 1, &lst_len_len);

        std::vector<T> result;
        size_t offset = pos + 1 + lst_len_len;
        for (size_t i = 0; i < lst_len; ++i)
        {
            size_t len = 0;
            T item = LuxProtoDef<T>::unpack(str, offset, &len);
            offset += len;
            result.push_back(item);
        }
        *used_len = offset - pos;
        return result;
    }
};

// std::map
template<typename K, typename V>
struct LuxProtoDef< std::map<K, V> >
{
    static void pack(std::string &str, const std::map<K, V> &dict)
    {
        str.push_back(LUX_HEADER_DICT);
        varint_pack(str, dict.size());
        for (const typename std::map<K, V>::value_type &pair : dict)
        {
            LuxProtoDef<K>::pack(str, pair.first);
            LuxProtoDef<V>::pack(str, pair.second);
        }
    }

    static std::map<K, V> unpack(const std::string &str, size_t pos, size_t *used_len)
    {
        uint8_t header = (uint8_t)str.at(pos);
        if (header != LUX_HEADER_DICT)
            throw_error(std::runtime_error, "header=0x%02X", header);

        size_t dict_len_len = 0;
        size_t dict_len = varint_unpack(str, pos + 1, &dict_len_len);

        std::map<K, V> result;
        size_t offset = pos + 1 + dict_len_len;
        for (size_t i = 0; i < dict_len; ++i)
        {
            size_t len = 0;
            K key = LuxProtoDef<K>::unpack(str, offset, &len);
            offset += len;
            V value = LuxProtoDef<V>::unpack(str, offset, &len);
            offset += len;
            result.insert(std::make_pair(key, value));
        }
        *used_len = offset - pos;
        return result;
    }
};
