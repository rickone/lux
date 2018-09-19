require "lux"

local function on_recv(socket, buffer)
    print("on_recv", socket, buffer)
    buffer:clear()
end

local function on_timer(socket, t)
    if (t.counter == 0) then
        socket:send("close")
    else
        socket:send("hello:"..t.counter)
    end
end

local factory = {
    tcp = socket_core.tcp_connect,
    udp = socket_core.udp_connect,
    unix = function(a1, a2)
        local socket = socket_core.unix_bind(a1)
        socket:connect(a2)
        return socket
    end,
    unix_ex = socket_core.unix_connect,
    kcp = function(...)
        local socket = socket_core.udp_connect(...)
        socket:set_reliable()
        return socket
    end
}

local params = {
    tcp = {"localhost", "8866"},
    udp = {"localhost", "8866"},
    unix = {"luxd-client.sock", "luxd-server.sock"},
    unix_ex = {"luxd.sock"},
    kcp = {"localhost", "8866"},
}

local extra = config.extra
local socket = factory[extra](table.unpack(params[extra]))
socket.on_connect = bind(print, "CONNECTED!!")
if extra == "kcp" then
    socket.on_recv_reliable = on_recv
else
    socket.on_recv = on_recv
end

local timer = lux_core.create_timer(100, 10)
timer.on_timer = bind(on_timer, socket)
