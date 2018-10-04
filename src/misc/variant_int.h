#pragma once

#include <cstddef> // size_t
#include <string>

uint32_t zigzag_encode32(int32_t value);
int32_t zigzag_decode32(uint32_t value);
uint64_t zigzag_encode64(int64_t value);
int64_t zigzag_decode64(uint64_t value);

void varint_pack(std::string &str, uint64_t value);
uint64_t varint_unpack(const std::string &str, size_t pos, size_t *read_len);
