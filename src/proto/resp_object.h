#pragma once

#include <string>
#include <vector>
#include <stdexcept>

enum class RespType
{
    RESP_NULL,
    RESP_SIMPLE_STRING,
    RESP_ERROR,
    RESP_INTEGER,
    RESP_BULK_STRING,
    RESP_ARRAY,
};

class RespObject
{
public:
    RespObject() = default;
    explicit RespObject(RespType type);
    virtual ~RespObject() = default;

    RespType type() const { return _type; }
    void set_type(RespType type) { _type = type; }

    const std::string &value() const { return _value; }
    void set_value(const std::string &value) { _value = value; }
    void set_value(const char *data, size_t len) { _value.assign(data, len); }

    void append_value(const std::string &value) { _value.append(value); }
    void append_value(const char *data, size_t len) { _value.append(data, len); }

    auto begin() const { return _array.begin(); }
    auto end() const { return _array.end(); }

    template<typename T>
    void set(T t)
    {
        static_assert(std::is_integral<T>::value, "RespObject.= need a integral type");
        set_value(std::to_string(t));
    }

    void set(const std::string &str);
    void set(const char *str);
    void set(bool val);

    template<typename T>
    RespObject & operator= (T t)
    {
        set(t);
        return *this;
    }

    template<typename T>
    void push(RespType type, T t)
    {
        RespObject obj(type);
        obj.set(t);
        _array.push_back(obj);
    }

    template<typename T, typename...A>
    void set_array(T t, A...args)
    {
        push(RespType::RESP_BULK_STRING, t);
        set_array(args...);
    }

    void set_array()
    {
    }

    void serialize(std::string &str);

    bool parse_line(const char *data, size_t len);
    bool parse_line(const std::string &str)
    {
        return parse_line(str.data(), str.size());
    }

    bool deserialize(const char *data, size_t len, size_t *used_len);
    bool deserialize(const std::string &str, size_t pos, size_t *used_len)
    {
        if (pos >= str.size())
            throw std::out_of_range("deserialize pos out of range");

        return deserialize(str.data() + pos, str.size() - pos, used_len);
    }

    operator std::string() const;
    friend std::ostream & operator<< (std::ostream &stream, const RespObject &obj);

private:
    RespType _type = RespType::RESP_NULL;
    std::string _value;
    std::vector<RespObject> _array;
    bool _parse_pending = false;
    size_t _unparsed_length = 0;
};
