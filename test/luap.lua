require "core"

local p = luap.new()

--[[
 * 0xC0 - nil
 * 0xC1 - false
 * 0xC2 - true
 * 0xC3 - number
 * 0xC4 - string
 * 0xC5 - table
 * 0xC6 - args
]]

local case = {1, 123, 10234, false, true, 3.14, "Hello", {10, 20, 30, 40}, {Name = "Rick"}}

for i, v in ipairs(case) do
    print("----------", v)
    p:pack(v)
    print(p:dump())
    print(p:unpack())
end

print("-----args-----")
p:pack_args(1, true, false, 3.14, "Hello", {Name = "Rick"})
print(p:dump())
print(p:unpack())
