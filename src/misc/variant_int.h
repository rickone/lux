#pragma once

#include <cstddef> // size_t
#include <string>

void variant_int_write(std::string &str, long value);
long variant_int_read(const std::string &str, size_t pos, size_t *read_len);
