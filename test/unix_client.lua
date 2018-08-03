require "lux"

local client = {name="client"}

function client:start()
    local socket
    if config.extra == "ex" then
        socket = socket_core.unix_connect_stream("luxd.sock")
    else
        socket = socket_core.unix_bind("luxd-client.sock")
        socket:connect("luxd-server.sock")
    end
    self.entity:add_component(socket)
    self.socket = socket
    
    self:subscribe(msg_type.socket_recv, "on_recv")
    self:set_timer("update", 100, 10)
end

function client:on_recv(buffer)
    print("on_recv", buffer)
    buffer:clear()
end

function client:update(t)
    local data = (t.counter == 0) and "close" or ("hello "..t.counter)
    print("send", data)

    self.socket:send(data)
    
    --if data == "close" then
        --self.entity:remove()
    --end
end

return client
