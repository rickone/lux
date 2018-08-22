require "lux"

local client = {}

function client:start()
    local socket = socket_core.udp_connect("localhost", "8866")
    self.entity:add_component(socket)
    self.socket = socket
    
    socket.on_recv = bind(self.on_socket_recv, self)
    
    local t = self.entity:add_timer(100, 10)
    t.on_timer = bind(self.update, self)
end

function client:on_socket_recv(socket, buffer)
    print("on_recv", buffer)
    buffer:clear()
end

function client:update(t)
    local data = (t.counter == 0) and "close" or ("hello:"..t.counter)
    print("send", data)

    self.socket:send(data)
    
    if data == "close" then
        self.entity:remove()
    end
end

return client