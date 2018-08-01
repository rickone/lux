require "core"

local client = {name="client"}

function client:start()
    local socket = socket_core.tcp_connect("::", "8866")
    self.entity:add_component(socket)
    
    local msgr = lux_core.create_msgr(msg_type.socket_recv, true)
    self.entity:add_component(msgr)
    self.msgr = msgr

    self:subscribe(msg_type.remote_call, "on_recv")
    self:set_timer("update", 100, 10)
end

function client:on_recv(...)
    print("on_recv", ...)
end

function client:update(t)
    self.msgr:send("ping", t.counter)
    if (t.counter == 0) then
        self.msgr:send("close")
    end
end

return client
