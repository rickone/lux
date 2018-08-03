require "lux"

--[[
 * 0xC0 - nil
 * 0xC1 - false
 * 0xC2 - true
 * 0xC3 - number
 * 0xC4 - string
 * 0xC5 - table
 ]]

local case = {1, 123, 10234, false, true, 3.14, "Hello", {10, 20, 30, 40}, {Name = "Rick"}}

local hex = function(s)
    return s:gsub(".", function(c) return string.format("%02X", string.byte(c)) end)
end

for i, v in ipairs(case) do
    print("----------", v)
    local s = lux_core.pack(v)
    print(hex(s))
    print(lux_core.unpack(s))
end

print("-----args-----")
local s = lux_core.pack(1, true, false, nil, 3.14, "Hello", {Name = "Rick"})
print(hex(s))
print(lux_core.unpack(s))

print("-----table-----")
local s = lux_core.pack({1, 100, 10000, -1, -100, -10000, true, false, 3.14, "Hello", {Name = "Rick"}})
print(hex(s))
print(tree(lux_core.unpack(s)))
