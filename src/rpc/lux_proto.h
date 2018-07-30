#pragma once

#include "buffer.h"

size_t lux_proto_pack(Buffer *buffer, lua_State *L, int index);
size_t lux_proto_pack_args(Buffer *buffer, lua_State *L, int n);

size_t lux_proto_pack_nil(Buffer *buffer);
size_t lux_proto_pack_bool(Buffer *buffer, bool value);
size_t lux_proto_pack_int(Buffer *buffer, long value);
size_t lux_proto_pack_number(Buffer *buffer, double value);
size_t lux_proto_pack_string(Buffer *buffer, const char *data, size_t len);
size_t lux_proto_pack_table_begin(Buffer *buffer);
size_t lux_proto_pack_table_end(Buffer *buffer);
size_t lux_proto_pack_args_begin(Buffer *buffer);
size_t lux_proto_pack_args_end(Buffer *buffer);

int lux_proto_unpack(lua_State *L, Buffer *buffer, size_t len);

enum LuxProtoType
{
    kLuxpType_Nil,
    kLuxpType_Boolean,
    kLuxpType_Integer,
    kLuxpType_Number,
    kLuxpType_String,
    kLuxpType_Table,
    kLuxpType_ArgList,
};

struct LuxProto
{
    int type;
    union {
        bool boolean;
        long integer;
        double number;
    } value;
    std::string str;
    std::vector<LuxProto> inside;
};

void lux_proto_unpack_object(LuxProto *lux_proto, Buffer *buffer, size_t len);
