require "core"

local client = {name="client"}

function client:start()
    local socket = socket_core.unix_connect("luxd.sock")
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
