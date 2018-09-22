local type = type
local getmetatable = getmetatable
local assert = assert
local tostring = tostring
local t_insert = table.insert
local t_concat = table.concat
local t_pack = table.pack
local s_format = string.format

local function pretty(x, depth)
    depth = 1 + (depth or 0)
    local t = type(x)
    if t == "table" then
        local mt = getmetatable(x)
        if mt then
            return tostring(x)
        end

        assert(depth < 64, "pretty to deep")

        local s = {}
        local mask = {}
        for i,v in ipairs(x) do
            t_insert(s, pretty(v, depth))
            mask[i] = true
        end
        for k,v in pairs(x) do
            if not mask[k] then
                t_insert(s, ("%s: %s"):format(pretty(k, depth), pretty(v, depth)))
            end
        end
        return s_format("{%s}", t_concat(s, ", "))
    end

    if t == "string" then
        return s_format("%q", x)
    end

    return tostring(x)
end

rawset(_G, "pretty", pretty)

local function __tostring(x)
    if type(x) == "table" then
        return pretty(x)
    end
    
    return tostring(x)
end

local lux_log = lux_core.log
local print = print

local function __print(...)
    local t = t_pack(...)
    local s = {}
    for i = 1, t.n do
        t_insert(s, __tostring(t[i]))
    end
    lux_log(t_concat(s, "\t"))
end

rawset(_G, "print", __print)
