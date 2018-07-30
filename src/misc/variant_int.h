#pragma once

#include <cstddef> // size_t

class Buffer;

size_t variant_int_write(Buffer *buffer, long value);
size_t variant_int_read(Buffer *buffer, long *value);
