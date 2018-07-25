#pragma once

#include <cstddef> // size_t
#include "error.h"

class Buffer;

void variant_int_write(long value, Buffer *buffer);
size_t variant_int_read(Buffer *buffer, long *result);

class VariantIntError : public std::runtime_error
{
public:
    explicit VariantIntError(const std::string &what) : std::runtime_error(what)
    {
    }
};
