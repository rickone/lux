local data_mgr = {}
local root = global("data_root")

function data_mgr.fetch(...)
    local node = root
    local args = table.pack(...)
    for i = 1, args.n do
        local k = args[i]
        local v = node[k]
        if v == nil then
            v = {}
            node[k] = v
        else
            local t = type(v)
            if t ~= "table" then
                error(("data_mgr.fetch error: #%d node type is '%s'"):format(i, t), 2)
            end
        end
        node = v
    end
    return node
end

function data_mgr.set(...)
    local args = table.pack(...)
    local n = args.n
    if n < 2 then
        error("data_mgr.set error: args number must >= 2", 2)
    end

    local node = data_mgr.fetch(root, table.unpack(args, 1, n - 2))
    node[args[n - 1]] = args[n]
end

function data_mgr.get(...)
    local node = root
    local args = table.pack(...)
    for i = 1, args.n do
        local k = args[i]
        node = node[k]
        if node == nil then
            return nil
        end
    end
    return node
end

return data_mgr
