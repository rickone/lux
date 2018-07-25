require "core"

local client = {name="client"}

function client:start()
    local socket = udp_socket.connect("::", "8866")
    self:add_component(socket)
    
    local kcp = socket_kcp.create()
    self:add_component(kcp)
    self.socket = kcp

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

    self.socket:send(data)
end

return client