local function proc_func(socket)
    local delegate = {}
    socket.delegate = delegate

    function delegate:on_recv(socket, buffer)
        print("on_recv", socket, buffer)
        buffer:clear()
    end
end

local sockets = {}
for i = 1, 2 do
    table.insert(sockets, socket_unix.fork("[stmd/worker:"..i.."]", proc_func))
    --table.insert(sockets, socket_unix.fork(nil, proc_func))
end

for i = 1, 10 do
    local data = "hey "..i
    sockets[1 + (i % 2)]:send(data)
end
