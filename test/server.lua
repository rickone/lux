require "lux"

local function on_recv(socket, buffer)
    local data = tostring(buffer)
    print("on_recv", data)

    if data == "close" then
        socket:close()
        return
    end

    socket:send("rsp:"..data)
    buffer:clear()
end

local function on_recvfrom(socket, buffer, addr)
    local data = tostring(buffer)
    print("on_recvfrom", data)

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
    udp = "udp://localhost:8866",
    udp_ex = "udp_ex://localhost:8866",
    unix = "unix://luxd-server.sock",
    unix_s = "unix_s://luxd.sock",
    kcp = "kcp://localhost:8866",
}

local service = require "service"
local socket = service.create(params[config.extra])
socket.on_accept = on_accept
socket.on_recvfrom = on_recvfrom
