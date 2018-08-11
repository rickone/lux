local function pretty(x, note)
    note = note or {}
    local t = type(x)
    if t == "table" then
        if note[x] then
            return "*"
        end
        note[x] = true

        local s = {}
        local mask = {}
        for i,v in ipairs(x) do
            table.insert(s, pretty(v, note))
            mask[i] = true
        end
        for k,v in pairs(x) do
            if not mask[k] then
                table.insert(s, ("%s: %s"):format(pretty(k, note), pretty(v, note)))
            end
        end
        return ("{%s}"):format(table.concat(s, ", "))
    end

    if t == "string" then
        return string.format("%q", x)
    end

    return tostring(x)
end

rawset(_G, "pretty", pretty)
