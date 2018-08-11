local mt = {}
setmetatable(_G, mt)

mt.__newindex = function(t, k, v)
    error("_G is protected!", 2)
end

local function global(name, init)
    local value = rawget(_G, name)
    if value ~= nil then
        return value
    end

    if init == nil then
        init = {}
    end

    value = init
    rawset(_G, name, value)
    return value
end

rawset(_G, "global", global)
