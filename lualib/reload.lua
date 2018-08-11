local require = require

local function reload(name)
    local old = package.loaded[name]
    if old == nil then
        return require(name)
    end

    package.loaded[name] = nil
    local new = require(name)
    if type(old) ~= "table" or type(new) ~= "table" then
        return new
    end

    for k,v in pairs(old) do
        old[k] = nil
    end
    for k,v in pairs(new) do
        old[k] = v
    end

    return old
end

rawset(_G, "reload", reload)
