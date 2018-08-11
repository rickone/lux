require "lux"

local connection = {}

function connection:on_socket_recv(socket, buffer)
    local data = tostring(buffer)

    print("on_recv", data)
    if data == "close" then
        self.entity:remove()
        return
    end

    socket:send("rsp:"..data)
    buffer:clear()
end

local server = {}

function server:start()
    local socket
    if config.extra == "ex" then
        socket = socket_core.udp_listen("::", "8866")
    else
        socket = socket_core.udp_bind("::", "8866")
    end

    self.entity:add_component(socket)
end

function server:on_socket_accept(listen_socket, socket)
    print("on_accept", socket)

    local ent = lux_core.create_entity()
    ent:add_component(socket)
    ent:add_component(connection)
end

function server:on_socket_recvfrom(socket, buffer, addr)
    local data = tostring(buffer)
    
    print("on_recv", data)
    socket:sendto("rsp:"..data, addr)
    
    if data == "close" then
        self.entity:remove()
    end
end

return server