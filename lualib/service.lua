local service = {}

local function parse_path(path)
    local protocol, name = path:match("^(.-)://(.*)$")
    local addr, port = name:match("^(.+):(.-)$")
    if addr == nil then
        addr = name
    end

    return protocol, addr, port
end

local server_factory = {
    tcp = socket_core.tcp_listen,
    udp = socket_core.udp_bind,
    udp_ex = socket_core.udp_listen,
    unix = socket_core.unix_bind,
    unix_s = socket_core.unix_listen,
    kcp = function(...)
        local socket = socket_core.udp_listen(...)
        socket:set_reliable()
        return socket
    end,
}

local client_factory = {
    tcp = socket_core.tcp_connect,
    udp = socket_core.udp_connect,
    udp_ex = socket_core.udp_connect,
    unix = function(a1, a2)
        local socket = socket_core.unix_bind(a1)
        socket:connect(a2)
        return socket
    end,
    unix_s = socket_core.unix_connect,
    kcp = function(...)
        local socket = socket_core.udp_connect(...)
        socket:set_reliable()
        return socket
    end
}

function service.create(path)
    local protocol, addr, port = parse_path(path)
    local f = server_factory[protocol]
    assert(f, "Unknown protocol : "..protocol)

    print("service.create -> ", path)
    return f(addr, port)
end

function service.connect(path)
    local protocol, addr, port = parse_path(path)
    local f = client_factory[protocol]
    assert(f, "Unknown protocol : "..protocol)

    print("service.connect -> ", path)
    return f(addr, port)
end

return service
