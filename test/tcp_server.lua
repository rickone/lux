require "lux"

local connection = {}

function connection:on_socket_recv(socket, buffer)
    local data = tostring(buffer)
    print("on_recv", socket, data)
    socket:send("rsp:"..data)
    buffer:clear()
end

local server = {}

function server:start()
    local socket = socket_core.tcp_listen("localhost", "8866")
    self.entity:add_component(socket)
end

function server:on_socket_accept(listen_socket, socket)
    print("on_accept", socket)

    local ent = lux_core.create_entity()
    ent:add_component(socket)
    ent:add_component(connection)
end

return server