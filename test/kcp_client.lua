require "lux"

local client = {}

function client:start()
    local socket = socket_core.udp_connect("::", "8866")
    self.entity:add_component(socket)
    
    local kcp = lux_core.create_kcp()
    self.entity:add_component(kcp)
    self.kcp = kcp

    local t = self.entity:add_timer(100, 10)
    t.on_timer = {self, "update"}
end

function client:on_kcp_recv(kcp, buffer)
    print("on_recv", buffer)
end

function client:update(t)
    local data = (t.counter == 0) and "close" or ("hello:"..t.counter)
    print("send", data)

    self.kcp:send(data)
end

return client