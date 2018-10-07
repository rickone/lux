require "lux"

local case = {1, 123, 10234, false, true, 3.14, "Hello", {10, 20, 30, 40}, {Name = "Rick"}}

local hex = function(s)
    return s:gsub(".", function(c) return string.format("%02X", string.byte(c)) end)
end

local pt = lux_core.create_lux_proto()

for i, v in ipairs(case) do
    print("----------", v)
    pt:clear()
    pt:pack(v)
    print(hex(pt.str))
    print(pt:unpack())
end

print("-----args-----")
pt:clear()
pt:pack(1, true, false, nil, 3.14, "Hello", {Name = "Rick"})
print(hex(pt.str))
print(pt:unpack())

print("-----table-----")
pt:clear()
pt:packlist({1, 100, 10000, -1, -100, -10000, true, false, 3.14, "Hello", {Name = "Rick"}})
print(hex(pt.str))
print(tree(pt:unpack()))
