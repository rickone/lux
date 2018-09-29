require "lux"

local function on_recv(socket, buffer)
    local data = tostring(buffer)
    print("on_recv", socket, data)

    if data == "close" then
        socket:close()
        return
    end

    socket:send("rsp:"..data)
    buffer:clear()
end

local function on_recvfrom(socket, buffer, addr)
    local data = tostring(buffer)
    print("on_recvfrom", socket, data)

    if data == "close" then
        socket:close()
        return
    end

    socket:sendto("rsp:"..data, addr)
end

local function on_accept(listen_socket, socket)
    print("on_accept", socket)

    socket.on_recv = on_recv
end

local params = {
    tcp = "tcp://localhost:8866",
    tcp_x = "tcp_x://localhost:8866",
    udp = "udp://localhost:8866",
    udp_x = "udp_x://localhost:8866",
    udp_r = "udp_r://localhost:8866",
    unix = "unix://luxd-server.sock",
    unix_s = "unix_s://luxd.sock",
}

local service = require "service"
local socket = service.create(params[config.extra])
socket.on_accept = on_accept
socket.on_recvfrom = on_recvfrom
