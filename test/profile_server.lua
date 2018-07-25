require "core"

local server = {}

function server:start(node, service)
    local socket = socket_tcp.new_service(node, service)
    socket.delegate = self

    self.listen_socket = socket
    self.sockets = {}
end

function server:on_accept(socket)
    socket.delegate = self
    self.sockets[socket.fd] = socket
end

function server:on_recv(socket, buffer)
    for str in tostring(buffer):gmatch("([^\n]+)") do
        local num = tonumber(str)
        local result = self:logic_test(num)
        socket:send(num.."="..result)
    end

    buffer:clear()
end

function server:logic_test(num)
    local result = {}
    --print(num)

    local m = math.floor(num ^ 0.5)
    for i = 2, m do
        while num % i == 0 do
            table.insert(result, i)
            num = math.floor(num / i)
        end
    end
    if num > 1 then
        table.insert(result, num)
    end

    return table.concat(result, "x")
end

server:start("::", "8866")
