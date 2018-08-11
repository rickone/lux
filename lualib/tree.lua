local signs = {{"├── ", "│   "}, {"└── ", "    "}}

local function tree(x, prefix, mem)
    prefix = prefix or "\n"
    mem = mem or {}

    local t = type(x)
    if t == "table" then
        local result = {}
        --table.insert(result, "*");

        local sid = 1
        local k, v = next(x, nil)
        while k ~= nil do
            local kn, vn = next(x, k)
            if kn == nil then
                sid = 2
            end

            table.insert(result, prefix)
            table.insert(result, signs[sid][1])

            local inner_prefix = prefix .. signs[sid][2]
            table.insert(result, ("%s: "):format(tree(k, inner_prefix)))
            table.insert(result, tree(v, inner_prefix))

            k = kn
            v = vn
        end
        return table.concat(result)
    end

    if t == "string" then
        return string.format("%q", x);
    end

    return tostring(x)
end

rawset(_G, "tree", tree)
