local agent = {}

function agent:start(gate_socket)
    gate_socket.delegate = self
    self.gate_socket = gate_socket
    self.sockets = {}
end

function agent:on_recv(socket, buffer)
    if socket == self.gate_socket then
        local fd = socket:pop_fd()
        local client_socket = socket_tcp.create(fd)

        client_socket.delegate = self
        self.sockets[fd] = client_socket
        return
    end

    local data = tostring(buffer)

    print("on_recv", socket, data)
    if data == "close" then
        self.sockets[socket.fd] = nil
        socket:close()
        return
    end

    socket:send("rsp "..data)
    buffer:clear()
end

agent:start(...)
