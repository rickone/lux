require "lux"

local function on_recv(socket, buffer)
    print("on_recv", socket, buffer)
    buffer:clear()
end

local function on_timer(socket, t)
    if (t.counter == 0) then
        socket:send("close")
    else
        socket:send("hello "..t.counter)
        socket:send("bye "..t.counter)
    end
end

local params = {
    tcp = "tcp://localhost:8866",
    tcp_x = "tcp_x://localhost:8866",
    udp = "udp://localhost:8866",
    udp_x = "udp_x://localhost:8866",
    udp_r = "udp_r://localhost:8866",
    unix = "unix://luxd-client.sock:luxd-server.sock",
    unix_s = "unix_s://luxd.sock",
}

local service = require "service"
local socket = service.connect(params[config.extra])
socket.on_connect = bind(print, "CONNECTED!!")
socket.on_recv = on_recv

local timer = lux_core.create_timer(100, 10, 100)
timer.on_timer = bind(on_timer, socket)
