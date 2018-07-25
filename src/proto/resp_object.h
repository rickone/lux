#pragma once

#include <memory> // shared_ptr
#include <string>
#include <vector>
#include "proto_base.h"

enum RespType
{
    kRespType_Null,
    kRespType_SimpleString,
    kRespType_Error,
    kRespType_Integer,
    kRespType_BulkString,
    kRespType_Array,
};

class RespObject : public ProtoBase
{
public:
    RespObject() = default;
    virtual ~RespObject() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<RespObject> create();
  
    void clear();
    void set_type(int type);
    void set_value(int type, const std::string &value);
    void set_value(int type, const char *data, size_t len);
    void add_array_member(const RespObject &obj);
    bool is_error();

    virtual int pack(lua_State *L) override;
    virtual int unpack(lua_State *L) override;
    virtual void serialize(Buffer *buffer) override;
    virtual bool deserialize(Buffer *buffer) override;

    int type() const { return _type; }
    const std::string & value() const { return _value; }
    const std::vector<RespObject> & array() const { return _array; }

private:
    bool parse_phase0(Buffer *buffer);
    bool parse_phase1(Buffer *buffer);
    bool parse_phase2(Buffer *buffer);

    int _type;
    std::string _value;
    std::vector<RespObject> _array;

    int _phase;
    size_t _value_left_length;
};
