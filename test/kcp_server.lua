require "core"

local connection = {name="connection"}

function connection:start()
    local kcp = socket_kcp.create()
    self:add_component(kcp)
    self.socket = kcp
    
    self:subscribe(msg_type.kcp_output, "on_recv")
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
    local socket = udp_socket_listener.create("::", "8866")
    self:add_component(socket)
    self.socket = socket

    self:subscribe(msg_type.socket_accept, "on_accept")
end

function server:on_accept(socket)
    print("on_accept", socket)

    local obj = object.create()
    obj:add_component(socket)
    obj:add_component(connection)
end

return server