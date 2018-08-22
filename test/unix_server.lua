require "lux"

local connection = {}

function connection:start(socket)
    self.entity:add_component(socket)

    socket.on_recv = bind(self.on_socket_recv, self)
end

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
        socket = socket_core.unix_listen("luxd.sock")
    else
        socket = socket_core.unix_bind("luxd-server.sock")
    end
    self.entity:add_component(socket)

    socket.on_accept = bind(self.on_socket_accept, self)
    socket.on_recvfrom = bind(self.on_socket_recvfrom, self)
end

function server:on_socket_accept(listen_socket, socket)
    print("on_accept", socket)

    local ent = lux_core.create_entity()
    ent:add_component(connection, socket)
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