require "lux"

local connection = {name="connection"}

function connection:start(socket)
    local msgr = lux_core.create_msgr(msg_type.socket_recv, true)
    self.entity:add_component(msgr)
    self.msgr = msgr

    self:subscribe(msg_type.remote_call, "on_recv")
end

function connection:on_recv(...)
    print("on_recv", ...)
    self.msgr:send("rsp", ...)
end

local server = {name="server"}

function server:start()
    local socket = socket_core.tcp_listen("::", "8866")
    self.entity:add_component(socket)

    self:subscribe(msg_type.socket_accept, "on_accept")
end

function server:on_accept(socket)
    print("on_accept", socket)

    local ent = lux_core.create_entity()
    ent:add_component(socket)
    ent:add_component(connection, socket)
end

return server