#pragma once

#include <cstddef> // size_t
#include <string>

class LuxProto
{
public:
    LuxProto() = default;

    template<typename T>
    void pack(T t)
    {
    }

    template<typename T>
    T unpack()
    {
        return T();
    }

    size_t pos() const { return _pos; }
    void set_pos(size_t pos) { _pos = pos; }

private:
    std::string _str;
    size_t _pos = 0;
};
