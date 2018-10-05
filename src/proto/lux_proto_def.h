#pragma once

#include "variant_int.h"
#include "error.h"

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
struct LuxProtoDef;

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


// const char *
template<>
struct LuxProtoDef<const char *>
{
    static void pack(std::string &str, const char *value)
    {
        size_t len = strlen(value);

        str.push_back(LUX_HEADER_STRING);
        varint_pack(str, len);
        str.append(value, len);
        str.push_back(0);
    }

    static const char * unpack(const std::string &str, size_t pos, size_t *used_len)
    {
        uint8_t header = (uint8_t)str.at(pos);
        if (header != LUX_HEADER_STRING)
            throw_error(std::runtime_error, "header=0x%02X", header);

        size_t var_len = 0;
        size_t str_len = varint_unpack(str, pos + 1, &var_len);
        const char *value = str.c_str() + pos + 1 + var_len;
        *used_len = 1 + var_len + str_len + 1;
        return value;
    }
};


// std::string
template<>
struct LuxProtoDef<std::string>
{
    static void pack(std::string &str, const std::string &value)
    {
        str.push_back(LUX_HEADER_STRING);
        varint_pack(str, value.size());
        str.append(value.data(), value.size());
        str.push_back(0);
    }

    static std::string unpack(const std::string &str, size_t pos, size_t *used_len)
    {
        uint8_t header = (uint8_t)str.at(pos);
        if (header != LUX_HEADER_STRING)
            throw_error(std::runtime_error, "header=0x%02X", header);

        size_t var_len = 0;
        size_t str_len = varint_unpack(str, pos + 1, &var_len);
        std::string value(str.c_str() + pos + 1 + var_len, str_len);
        *used_len = 1 + var_len + str_len + 1;
        return value;
    }
};
