#pragma once

#include <memory> // shared_ptr
#include <string>
#include <vector>
#include "proto_base.h"

enum LuapType
{
    kLuapType_Nil,
    kLuapType_Boolean,
    kLuapType_Integer,
    kLuapType_Number,
    kLuapType_String,
    kLuapType_Table,
    kLuapType_Args,
};

union LuapValue
{
    bool boolean;
    long integer;
    double number;
};

class LuapObject : public ProtoBase
{
public:
    LuapObject() = default;
    virtual ~LuapObject() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<LuapObject> create();
    
    void clear();
    void set_nil();
    void set_boolean(bool value);
    void set_integer(long value);
    void set_number(double value);
    void set_string(const char *data, size_t len);
    void set_table();
    void table_set(const LuapObject &key, const LuapObject &value);
    void set_args();
    void args_push(const LuapObject &obj);

    virtual void serialize(Buffer *buffer) override;
    virtual bool deserialize(Buffer *buffer) override;
    virtual int pack(lua_State *L) override;
    virtual int unpack(lua_State *L) override;
    int pack_args(lua_State *L);

    int type() const { return _type; }
    bool boolean() const { return _value.boolean; }
    long integer() const { return _value.integer; }
    double number() const { return _value.number; }
    const std::string & string() const { return _str_value; }

private:
    bool parse_phase0(Buffer *buffer);
    bool parse_phase1(Buffer *buffer);
    bool parse_phase2(Buffer *buffer);
    
    int _type;
    LuapValue _value;
    std::string _str_value;
    std::vector<LuapObject> _table;

    int _phase;
    size_t _value_left_length;
};
