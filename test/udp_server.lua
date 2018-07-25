require "core"

local connection = {name="connection"}

function connection:start()
    self.socket = self:find_component("socket")
    self:subscribe(msg_type.socket_recv, "on_recv")
end

function connection:on_recv(buffer)
    local data = tostring(buffer)

    print("on_recv", data)
    if data == "close" then
        self:remove_entity()
        return
    end

    self.socket:send("rsp "..data)
    buffer:clear()
end

local server = {name="server"}

function server:start()
    local socket
    if config.extra == "ex" then
        socket = udp_socket_listener.create("::", "8866")
        self:subscribe(msg_type.socket_accept, "on_accept")
    else
        socket = udp_socket.bind("::", "8866")
        self:subscribe(msg_type.socket_recv, "on_recv")
    end

    self:add_component(socket)
    self.socket = socket
end

function server:on_accept(socket)
    print("on_accept", socket)

    local obj = object.create()
    obj:add_component(socket)
    obj:add_component(connection)
end

function server:on_recv(buffer, addr)
    local data = tostring(buffer)
    
    print("on_recv", data)
    self.socket:sendto("rsp "..data, addr)
    buffer:clear()
    
    if data == "close" then
        self:remove_entity()
    end
end

return server