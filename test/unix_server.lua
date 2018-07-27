require "core"

local connection = {name="connection"}

function connection:start(socket)
    self.socket = socket
    self:subscribe(msg_type.socket_recv, "on_recv")
end

function connection:on_recv(buffer)
    local data = tostring(buffer)

    print("on_recv", data)
    if data == "close" then
        self.entity:remove()
        return
    end

    self.socket:send("rsp "..data)
    buffer:clear()
end

local server = {name="server"}

function server:start()
    local socket = socket_core.unix_listen("luxd.sock")
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