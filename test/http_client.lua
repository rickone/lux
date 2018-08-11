require "lux"

local client = {}

function client:init(host, request, delegate)
    local port = 80
    local i = host:find(":")
    if i then
        port = host:sub(i + 1, -1)
        host = host:sub(1, i - 1)
    end
    self.host = host
    self.port = port
    self.request = request
    self.delegate = delegate
end

function client:start()
    local socket = socket_core.tcp_connect(self.host, self.port)
    self.entity:add_component(socket)
    self.socket = socket
end

function client:on_socket_connect(socket)
    if self.delegate then
        self.delegate:on_http_connect()
    end

    if self.request then
        self.request:send(socket)
    end
end

function client:on_socket_recv(socket, buffer)
    if self.delegate then
        self.delegate:on_http_respond(buffer)
    end
end

function client:on_socket_close(socket)
    if self.delegate then
        self.delegate:on_http_close()
    end
    self:close()
end

function client:send(data)
    self.socket:send(data)
end

function client:close()
    self.entity:remove()
end

return client