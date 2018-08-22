require "lux"

local client = {}

function client:start()
    local socket = socket_core.tcp_connect("localhost", "8866")
    self.entity:add_component(socket)
    self.socket = socket

    socket.on_connect = bind(self.on_socket_connect, self)
    socket.on_recv = bind(self.on_socket_recv, self)

    local t = self.entity:add_timer(100, 10)
    t.on_timer = bind(self.update, self)
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
