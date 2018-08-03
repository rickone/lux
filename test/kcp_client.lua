require "lux"

local client = {name="client"}

function client:start()
    local socket = socket_core.udp_connect("::", "8866")
    self.entity:add_component(socket)
    
    local kcp = lux_core.create_kcp()
    self.entity:add_component(kcp)
    self.kcp = kcp

    self:subscribe(msg_type.kcp_output, "on_recv")
    self:set_timer("update", 100, 10)
end

function client:on_recv(buffer)
    print("on_recv", buffer)
    buffer:clear()
end

function client:update(t)
    local data = (t.counter == 0) and "close" or ("hello "..t.counter)
    print("send", data)

    self.kcp:send(data)
end

return client