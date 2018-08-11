require "lux"

local connection = {}

function connection:start()
    local kcp = lux_core.create_kcp()
    self.entity:add_component(kcp)
end

function connection:on_kcp_recv(kcp, buffer)
    local data = tostring(buffer)

    print("on_recv", data)

    kcp:send("rsp:"..data)
end

local server = {}

function server:start()
    local socket = socket_core.udp_listen("::", "8866")
    self.entity:add_component(socket)
end

function server:on_socket_accept(listen_socket, socket)
    print("on_accept", socket)

    local ent = lux_core.create_entity()
    ent:add_component(socket)
    ent:add_component(connection)
end

return server