require "lux"

local function proc_func()
    local agent = {}

    function agent:start(socket)
        self.entity:add_component(socket)
        socket.on_recv = bind(self.on_recv, self)
    end

    function agent:on_recv(socket, buffer)
        print("on_recv", socket, buffer)
        buffer:clear()
    end

    return agent
end

local sockets = {}
for i = 1, 2 do
    table.insert(sockets, lux_core.fork("[luxd/worker:"..i.."]", proc_func))
end

for i = 1, 10 do
    local data = "hey "..i
    sockets[1 + (i % 2)]:send(data)
end
