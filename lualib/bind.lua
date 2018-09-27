local select = select
local assert = assert

local binds = {
    function(func, a1)
        return function(...)
            return func(a1, ...)
        end
    end,
    function(func, a1, a2)
        return function(...)
            return func(a1, a2, ...)
        end
    end,
    function(func, a1, a2, a3)
        return function(...)
            return func(a1, a2, a3, ...)
        end
    end,
    function(func, a1, a2, a3, a4)
        return function(...)
            return func(a1, a2, a3, a4, ...)
        end
    end,
}

local function bind(func, ...)
    local n = select('#', ...)
    local f = assert(binds[n], "bind number of args too much")
    return f(func, ...)
end

rawset(_G, "bind", bind)
