#pragma once

#include <memory> // shared_ptr
#include <string>
#include <vector>
#include "redis_proto_def.h"

class RedisObject
{
public:
    virtual ~RedisObject() = default;
    virtual void serialize(std::string &str) const = 0;
};

class RedisNull : public RedisObject
{
public:
    explicit RedisNull(char header) : _header(header) {}
    virtual ~RedisNull() = default;

    virtual void serialize(std::string &str) const override;

private:
    char _header;
};

class RedisSimpleString : public RedisObject
{
public:
    RedisSimpleString(const std::string &str) : _str(str) {}
    RedisSimpleString(const std::string &str, size_t pos, size_t len) : _str(str, pos, len) {}
    virtual ~RedisSimpleString() = default;

    virtual void serialize(std::string &str) const override;

private:
    std::string _str;
};

class RedisError : public RedisObject
{
public:
    RedisError(const std::string &str) : _str(str) {}
    RedisError(const std::string &str, size_t pos, size_t len) : _str(str, pos, len) {}
    virtual ~RedisError() = default;

    virtual void serialize(std::string &str) const override;

private:
    std::string _str;
};

template<typename T>
class RedisInteger : public RedisObject
{
public:
    RedisInteger(T val) : _val(val)
    {
        static_assert(std::is_integral<T>::value, "RedisInteger<T> need a integral type");
    }

    virtual ~RedisInteger() = default;

    virtual void serialize(std::string &str) const override
    {
        lux::redis_pack_integer(str, _val);
    }

private:
    T _val;
};

class RedisBulkString : public RedisObject
{
public:
    RedisBulkString(const char *data, size_t len) : _str(data, len) {}
    RedisBulkString(const std::string &str, size_t pos, size_t len) : _str(str, pos, len) {}
    virtual ~RedisBulkString() = default;

    virtual void serialize(std::string &str) const override;

private:
    std::string _str;
};

class RedisArray : public RedisObject
{
public:
    RedisArray() = default;
    virtual ~RedisArray() = default;

    void push(const std::shared_ptr<RedisObject> &obj);
    void push(const std::string &str);
    void push_error(const std::string &str);
    void push_bulk_string(const char *data, size_t len);

    template<typename T>
    void push(T t)
    {
        _array.push_back(std::make_shared< RedisInteger<T> >(t));
    }

    virtual void serialize(std::string &str) const override;

    std::vector< std::shared_ptr<RedisObject> > &body() { return _array; }

private:
    std::vector< std::shared_ptr<RedisObject> > _array;
};
