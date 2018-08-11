local function class(name, base)
    local cls = rawget(_G, name)
    local tp = type(cls)
    if tp == "table" then
        for k,v in pairs(cls) do
            cls[k] = nil
        end
        return cls
    end

    if tp ~= "nil" then
        error(("class type error: '%s' type(%s)"):format(name, tp), 2)
    end

    local mt = {}
    cls = setmetatable({}, mt)

    rawset(_G, name, cls)

    if base then
        mt.__index = base
        cls.super = base
    end

    local obj_mt = {}
    obj_mt.__index = cls

    mt.__call = function(t, ...)
        local obj = setmetatable({}, obj_mt)
        local init = obj.init
        if init then
            init(obj, ...)
        end
        return obj
    end

    return cls
end

rawset(_G, "class", class)
