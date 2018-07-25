local split = function(str, sp)
    local result = {}
    local i, j, len = 1, 1, sp:len()
    while true do
        j = str:find(sp, i)
        if j == nil then
            table.insert(result, str:sub(i))
            break
        end
        table.insert(result, str:sub(i, j - 1))

        i = j + len
    end
    return result
end

return split