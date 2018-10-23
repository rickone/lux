require "lux"

local case = {1, 123, 10234, false, true, 3.14, "Hello", {10, 20, 30, 40}, {"Name", "Rick"}}
local repl = { ["\r"] = "\\r", ["\n"] = "\\n" }

local hex = function(s)
    return s:gsub("[\r\n]", repl)
end

local pt = lux_core.create_redis_proto()

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
pt:pack({1, 100, 10000, -1, -100, -10000, true, false, 3.14, "Hello", {"Name", "Rick"}})
print(hex(pt.str))
print(tree(pt:unpack()))
