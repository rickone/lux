#pragma once

#include <cstddef> // size_t
#include "error.h"

class Buffer;

size_t variant_int_write(Buffer *buffer, long value);
bool variant_int_read(Buffer *buffer, long *result, size_t *used_len);

class VariantIntError : public std::runtime_error
{
public:
    explicit VariantIntError(const std::string &what) : std::runtime_error(what)
    {
    }
};
