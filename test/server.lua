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

local factory = {
    tcp = socket_core.tcp_listen,
    udp = socket_core.udp_bind,
    udp_ex = socket_core.udp_listen,
    unix = socket_core.unix_bind,
    unix_ex = socket_core.unix_listen,
    kcp = function(...)
        local socket = socket_core.udp_listen(...)
        socket:set_reliable()
        return socket
    end,
}

local params = {
    tcp = {"localhost", "8866"},
    udp = {"localhost", "8866"},
    udp_ex = {"localhost", "8866"},
    unix = {"luxd-server.sock"},
    unix_ex = {"luxd.sock"},
    kcp = {"localhost", "8866"},
}

local extra = config.extra
local socket = factory[extra](table.unpack(params[extra]))
socket.on_accept = on_accept
socket.on_recvfrom = on_recvfrom
