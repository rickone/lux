require "lux"

local connection = {name="connection"}

function connection:start()
    local kcp = lux_core.create_kcp()
    self.entity:add_component(kcp)
    self.kcp = kcp
    
    self:subscribe(msg_type.kcp_output, "on_recv")
end

function connection:on_recv(buffer)
    local data = tostring(buffer)

    print("on_recv", data)

    self.kcp:send("rsp "..data)
    buffer:clear()
end

local server = {name="server"}

function server:start()
    local socket = socket_core.udp_listen("::", "8866")
    self.entity:add_component(socket)
    self.socket = socket

    self:subscribe(msg_type.socket_accept, "on_accept")
end

function server:on_accept(socket)
    print("on_accept", socket)

    local ent = lux_core.create_entity()
    ent:add_component(socket)
    ent:add_component(connection)
end

return server