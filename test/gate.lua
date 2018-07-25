require "core"

local gate = {}
local agent_chunk = loadfile("./agent.lua", "bt")

function gate:start(node, service)
    local socket = socket_tcp.new_service(node, service)
    socket.delegate = self
    self.listen_socket = socket

    local agents = {}
    for i = 1, 4 do
        table.insert(agents, socket_unix.fork("[agent:"..i.."]", agent_chunk))
    end
    self.agents = agents
end

function gate:on_accept(socket)
    print("accept", socket)
    local agents = self.agents
    local agent = agents[math.random(#agents)]

    agent:push_fd(socket:detach())
    agent:send("+")
end

gate:start("::", "8866")
