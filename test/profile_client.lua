require "core"

local client = {}

function client:start(node, service)
    local socket = socket_tcp.connect(node, service)
    socket.delegate = self

    self.socket = socket

    local t = timer.create(10, 6000)
    t.delegate = self
end

function client:on_recv(socket, buffer)
    --print("on_recv", buffer)
    buffer:clear()
end

function client:on_timer(t)
    if t.counter > 1 then
        self.socket:send(tostring(t.counter).."\n")
    else
        print("done")
        os.exit()
    end
end

local function worker()
    client:start("::", "8866")
end

for i = 1, 20 do
    socket_unix.fork("[worker:"..i.."]", worker)
end
