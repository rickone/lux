local tpack = table.pack
local tunpack = table.unpack

local function bind(func, ...)
    local params = tpack(...)
    return function(...)
        return func(tunpack(params), ...)
    end
end

rawset(_G, "bind", bind)
