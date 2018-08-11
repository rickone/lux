require "lux"

local client = {}

function client:start()
    local socket = socket_core.tcp_connect("www.baidu.com", "443")
    self.entity:add_component(socket)
    self.socket = socket

    --self:set_timer("update", 100, 10)
end

function client:on_socket_connect(socket)
    print("CONNECTED!!")
end

function client:on_socket_recv(socket, buffer)
    print("on_recv", socket, buffer)
    buffer:clear()
end

function client:update(t)
    self.socket:send("hello:"..t.counter)
    if (t.counter == 0) then
        self.socket:send("close")
    end
end

return client
