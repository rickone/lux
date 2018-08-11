local hooks = {}

local function hook(tb, name, func)
    local org = assert(tb[name])

    if func == nil then
        tb[name] = assert(hooks[org])
        hooks[org] = nil
        return
    end

    tb[name] = func
    hooks[func] = org
end

rawset(_G, "hook", hook)
