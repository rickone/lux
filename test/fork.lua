require "lux"

local function proc_func(socket)
    function socket:on_recv(buffer)
        print("on_recv", self, buffer)
        buffer:clear()
    end
end

local sockets = {}
for i = 1, 2 do
    table.insert(sockets, lux_core.fork("[luxd/worker:"..i.."]", proc_func))
end

for i = 1, 10 do
    local data = "hey "..i
    sockets[1 + (i % 2)]:send(data)
end
