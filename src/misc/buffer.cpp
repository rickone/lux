#include "buffer.h"
#include <algorithm> // std::min
#include <cstring> // memcpy strndup
#include <cstdlib> // free

Buffer::Buffer(size_t min_alloc_size) : _data(), _max_size(), _min_alloc_size(min_alloc_size), _mask(), _front_pos(), _back_pos()
{
}

Buffer::Buffer(const Buffer &other) : _data(), _max_size(other._max_size), _min_alloc_size(other._min_alloc_size),
    _mask(other._mask), _front_pos(other._front_pos), _back_pos(other._back_pos)
{
    if (other._data)
    {
#ifdef _WIN32
        _data = (char *)malloc(other._max_size);
        memcpy(_data, other._data, other._max_size);
#else
        _data = strndup(other._data, other._max_size);
#endif
    }
}

Buffer::Buffer(Buffer &&other) : _data(other._data), _max_size(other._max_size), _min_alloc_size(other._min_alloc_size),
    _mask(other._mask), _front_pos(other._front_pos), _back_pos(other._back_pos)
{
    other._data = nullptr;
    other._max_size = 0;
    other._min_alloc_size = 0;
    other._mask = 0;
    other._front_pos = 0;
    other._back_pos = 0;
}

Buffer::~Buffer()
{
    if (_data)
    {
        free(_data);
        _data = nullptr;
    }
}

void Buffer::new_class(lua_State *L)
{
    lua_new_class(L, Buffer);

    lua_newtable(L);
    {
        lua_method(L, clear);
        lua_method(L, size);
        lua_method(L, max_size);
        lua_method(L, resize);
        lua_method(L, empty);
        lua_method(L, push_string);
        lua_method(L, pop_string);
        lua_method(L, find);
        lua_method(L, dump);
    }
    lua_setfield(L, -2, "__method");

    lua_newtable(L);
    {
        lua_property(L, min_alloc_size);
    }
    lua_setfield(L, -2, "__property");

    lua_push_method(L, tostring);
    lua_setfield(L, -2, "__tostring");

    lua_lib(L, "lux_core");
    {
        lua_set_method(L, "create_buffer", create);
    }
    lua_pop(L, 1);
}

std::shared_ptr<Buffer> Buffer::create()
{
    return std::shared_ptr<Buffer>(new Buffer());
}

void Buffer::clear()
{
    _front_pos = 0;
    _back_pos = 0;
}

const char * Buffer::data(size_t pos/* = 0*/) const
{
    return _data + ((_front_pos + pos) & _mask);
}

size_t Buffer::size() const
{
    return _back_pos - _front_pos;
}

size_t Buffer::max_size() const
{
    return _max_size;
}

void Buffer::resize(size_t len)
{
    size_t sz = size();
    if (len > sz)
    {
        realloc(len);
        _back_pos += (len - sz);
    }
    else
    {
        _front_pos += (sz - len);
    }
}

bool Buffer::empty()
{
    return _front_pos == _back_pos;
}

void Buffer::set(size_t pos, const char *data, size_t len)
{
    size_t sz = size();
    logic_assert(pos + len <= sz, "pos = %u, len = %u, size = %u", pos, len, sz);

    size_t offset = (_front_pos + pos) & _mask;
    size_t first_part = std::min(len, _max_size - offset);

    memcpy(_data + offset, data, first_part);
    memcpy(_data, data + first_part, len - first_part);
}

void Buffer::get(size_t pos, char *data, size_t len) const
{
    size_t sz = size();
    logic_assert(pos + len <= sz, "pos = %u, len = %u, size = %u", pos, len, sz);

    size_t offset = (_front_pos + pos) & _mask;
    size_t first_part = std::min(len, _max_size - offset);

    memcpy(data, _data + offset, first_part);
    memcpy(data + first_part, _data, len - first_part);
}

void Buffer::get_string(size_t pos, std::string &str, size_t len) const
{
    size_t sz = size();
    logic_assert(pos + len <= sz, "pos = %u, len = %u, size = %u", pos, len, sz);

    size_t offset = (_front_pos + pos) & _mask;
    size_t first_part = std::min(len, _max_size - offset);

    str.append(_data + offset, first_part);
    str.append(_data, len - first_part);
}

std::string Buffer::get_string(size_t pos, size_t len) const
{
    std::string str;
    get_string(pos, str, len);
    return str;
}

void Buffer::push(const char *data, size_t len)
{
    size_t sz = size();
    realloc(sz + len);

    _back_pos += len;
    if (data)
        set(sz, data, len);
}

void Buffer::pop(char *data, size_t len)
{
    size_t sz = size();
    logic_assert(len <= sz, "len = %u, size = %u", len, sz);

    if (data)
        get(0, data, len);
    _front_pos += len;
}

size_t Buffer::var(size_t len)
{
    size_t sz = size();
    realloc(sz + len);

    _back_pos += len;
    return sz;
}

void Buffer::push_string(const std::string &str)
{
    push(str.data(), str.size());
}

std::string Buffer::pop_string(size_t len)
{
    std::string str = get_string(0, len);
    pop(nullptr, str.size());
    return str;
}

int Buffer::find(size_t pos, const std::string &str)
{
    // KMP later
    size_t sz = size();
    size_t str_len = str.size();
    for (; pos < sz; ++pos)
    {
        if (*data(pos) != str.at(0))
            continue;

        size_t offset = 1;
        for (; offset < str_len; ++offset)
        {
            if (*data(pos + offset) != str.at(offset))
                break;
        }

        if (offset == str_len)
            return (int)pos;
    }

    return -1;
}

std::pair<char *, size_t> Buffer::front()
{
    size_t sz = size();
    size_t offset = _front_pos & _mask;
    size_t len = std::min(sz, _max_size - offset);

    return std::make_pair(_data + offset, len);
}

std::pair<char *, size_t> Buffer::back()
{
    size_t sz = size();
    if (sz == _max_size)
        realloc(sz + 1);

    size_t left_size = _max_size - sz;
    size_t offset = _back_pos & _mask;
    size_t len = std::min(left_size, _max_size - offset);

    return std::make_pair(_data + offset, len);
}

std::string Buffer::dump()
{
    std::string str;
    size_t sz = size();

    for (size_t pos = 0; pos < sz; ++pos)
    {
        unsigned char c = (unsigned char)*data(pos);
        static char temp[8];
        snprintf(temp, sizeof(temp), "%02x", c);

        str.append(temp);
    }

    return str;
}

std::string Buffer::tostring()
{
    size_t sz = size();
    return get_string(0, sz);
}

void Buffer::realloc(size_t use_length)
{
    if (use_length <= _max_size)
        return;

    size_t alloc_length = std::max(use_length, _min_alloc_size);
    size_t target_length = std::max((_max_size << 1), (size_t)1);
    while (alloc_length > target_length)
        target_length <<= 1;

/*
front &= _mask
back &= _mask

case 1:
[[...<front>aabb<back>...] .....new..... ]
do nothing

case 2:
[[bb<back>......<front>aa] .....new..... ]
move bb after aa
back += max_size

*/

    size_t sz = size();
    _data = (char *)::realloc(_data, target_length);
    _front_pos &= _mask;
    _back_pos &= _mask;
    if (_back_pos < _front_pos || sz == _max_size)
    {
        memcpy(_data + _max_size, _data, _back_pos);
        _back_pos += _max_size;
    }
    _max_size = target_length;
    _mask = target_length - 1;
}
