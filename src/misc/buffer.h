#pragma once

#include <string>
#include "lua_port.h"

class Buffer : public LuaObject
{
public:
    explicit Buffer(size_t min_alloc_size = 512);
    Buffer(const Buffer &other);
    Buffer(Buffer &&other);
    virtual ~Buffer();

    static void new_class(lua_State *L);

    void clear();
    const char * data(size_t pos = 0) const;
    size_t size() const;
    size_t max_size() const;
    void resize(size_t len);
    bool empty();

    void set(size_t pos, const char *data, size_t len);
    void get(size_t pos, char *data, size_t len) const;
    void get_string(size_t pos, std::string &str, size_t len) const;
    std::string get_string(size_t pos, size_t len) const;

    void push(const char *data, size_t len);
    void pop(char *data, size_t len);
    size_t var(size_t len);
    void push_string(const std::string &str);
    std::string pop_string(size_t len);
    int find(const std::string &str);

    std::pair<char *, size_t> front();
    std::pair<char *, size_t> back();

    std::string dump();
    std::string tostring();

    size_t min_alloc_size() const { return _min_alloc_size; }
    void set_min_alloc_size(size_t min_alloc_size) { _min_alloc_size = min_alloc_size; }

protected:
    void realloc(size_t len);

    char  *_data;
    size_t _max_size;
    size_t _min_alloc_size;
    size_t _mask;
    size_t _front_pos;
    size_t _back_pos;
};
