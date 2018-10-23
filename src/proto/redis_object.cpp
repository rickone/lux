#include "redis_object.h"
#include "redis_proto_def.h"

using namespace lux;

void RedisNull::serialize(std::string &str) const
{
    if (_header == REDIS_HEADER_BULK_STRING)
    {
        redis_pack_bulk_string(str, nullptr, 0);
    }
    else
    {
        redis_pack_array(str, 0, nullptr, 0);
    }
}

void RedisSimpleString::serialize(std::string &str) const
{
    redis_pack_simple_string(str, _str);
}

void RedisError::serialize(std::string &str) const
{
    redis_pack_error(str, _str);
}

void RedisBulkString::serialize(std::string &str) const
{
    redis_pack_bulk_string(str, _str.data(), _str.size());
}

void RedisArray::push(const std::shared_ptr<RedisObject> &obj)
{
    _array.push_back(obj);
}

void RedisArray::push(const std::string &str)
{
    _array.push_back(std::make_shared<RedisSimpleString>(str));
}

void RedisArray::push_error(const std::string &str)
{
    _array.push_back(std::make_shared<RedisError>(str));
}

void RedisArray::push_bulk_string(const char *data, size_t len)
{
    _array.push_back(std::make_shared<RedisBulkString>(data, len));
}

void RedisArray::serialize(std::string &str) const
{
    std::string body;
    for (auto &obj : _array)
    {
        obj->serialize(body);
    }
    redis_pack_array(str, _array.size(), body.data(), body.size());
}
