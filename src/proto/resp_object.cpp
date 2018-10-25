#include "resp_object.h"
#include <cassert>
#include <cstring>

RespObject::RespObject(RespType type) : _type(type)
{
}

void RespObject::set(const std::string &str)
{
    set_value(str);
}

void RespObject::set(const char *str)
{
    set_value(str);
}

void RespObject::set(bool val)
{
    set_value(val ? "1" : "0");
}

void RespObject::serialize(std::string& str)
{
    switch (_type)
    {
        case RespType::RESP_NULL:
        {
            str.append("$-1\r\n", 5);
            break;
        }

        case RespType::RESP_SIMPLE_STRING:
        {
            str.push_back('+');
            str.append(_value);
            str.append("\r\n", 2);
            break;
        }

        case RespType::RESP_ERROR:
        {
            str.push_back('-');
            str.append(_value);
            str.append("\r\n", 2);
            break;
        }

        case RespType::RESP_INTEGER:
        {
            str.push_back(':');
            str.append(_value);
            str.append("\r\n", 2);
            break;
        }

        case RespType::RESP_BULK_STRING:
        {
            str.push_back('$');
            str.append(std::to_string(_value.size()));
            str.append("\r\n", 2);
            str.append(_value);
            str.append("\r\n", 2);
            break;
        }

        case RespType::RESP_ARRAY:
        {
            str.push_back('*');
            str.append(std::to_string(_array.size()));
            str.append("\r\n", 2);
            for (auto &obj : _array)
            {
                obj.serialize(str);
            }
            break;
        }

        default:
            assert(false);
    }
}

bool RespObject::parse_line(const char *data, size_t len)
{
    if (_parse_pending)
    {
        switch (_type)
        {
            case RespType::RESP_BULK_STRING:
            {
                if (len <= _unparsed_length)
                {
                    append_value(data, len);
                    _unparsed_length -= len;
                    return false;
                }

                assert(len == _unparsed_length + 2);
                append_value(data, _unparsed_length);

                _unparsed_length = 0;
                _parse_pending = false;
                return true;
            }

            case RespType::RESP_ARRAY:
            {
                if (!_array.back().parse_line(data, len))
                    return false;

                --_unparsed_length;
                if (_unparsed_length > 0)
                {
                    push(RespType::RESP_NULL, "");
                    return false;
                }

                _parse_pending = false;
                return true;
            }

            default:
                throw std::runtime_error(std::string("RESP Type invalid: ") + std::to_string((int)_type));
        }
    }

    switch (*data)
    {
        case '+':
        {
            set_type(RespType::RESP_SIMPLE_STRING);
            set_value(data + 1, len - 3);
            return true;
        }

        case '-':
        {
            set_type(RespType::RESP_ERROR);
            set_value(data + 1, len - 3);
            return true;
        }

        case ':':
        {
            set_type(RespType::RESP_INTEGER);
            set_value(data + 1, len - 3);
            return true;
        }

        case '$':
        {
            set_type(RespType::RESP_BULK_STRING);

            int str_len = std::stoi(std::string(data + 1, len - 3));
            if (str_len > 0)
            {
                _unparsed_length = (size_t)str_len;
                _parse_pending = true;
                break;
            }

            if (str_len < 0)
                set_type(RespType::RESP_NULL);

            return true;
        }

        case '*':
        {
            set_type(RespType::RESP_ARRAY);

            int count = std::stoi(std::string(data + 1, len - 3));
            if (count > 0)
            {
                _unparsed_length = (size_t)count;
                _parse_pending = true;
                push(RespType::RESP_NULL, "");
                break;
            }

            if (count < 0)
                set_type(RespType::RESP_NULL);

            return true;
        }

        default:
            throw std::runtime_error(std::string("Unknown RESP header: ") + *data);
    }

    return false;
}

bool RespObject::deserialize(const char *data, size_t len, size_t *used_len)
{
    const char *offset = data;
    for (;;)
    {
        const char *tail = strstr(offset, "\r\n");
        if (tail == nullptr)
            break;

        bool ret = parse_line(offset, tail - offset + 2);
        if (ret)
        {
            *used_len = tail - data + 2;
            return true;
        }

        offset = tail + 2;
    }

    *used_len = offset - data;
    return false;
}

RespObject::operator std::string() const
{
    std::string result;

    switch (_type)
    {
        case RespType::RESP_NULL:
        {
            result.append("(nil)", 5);
            break;
        }

        case RespType::RESP_SIMPLE_STRING:
        case RespType::RESP_BULK_STRING:
        {
            // TODO string escaping
            result.push_back('"');
            result.append(_value);
            result.push_back('"');
            break;
        }

        case RespType::RESP_ERROR:
        {
            result.append(_value);
            break;
        }

        case RespType::RESP_INTEGER:
        {
            result.append("(integer) ", 10);
            result.append(_value);
            break;
        }

        case RespType::RESP_ARRAY:
        {
            int i = 0;
            for (auto &obj : _array)
            {
                result.append(std::to_string(++i));
                result.append(") ");
                result.append(obj);
                result.append("\r\n", 2);
            }
            break;
        }

        default:
            assert(false);
    }
    return result;
}

std::ostream & operator<< (std::ostream &stream, const RespObject &obj)
{
    return stream << (std::string)obj;
}
