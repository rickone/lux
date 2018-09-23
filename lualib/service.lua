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
    tcp_x = function(...)
        local socket = socket_core.tcp_listen(...)
        socket:set_package_mode()
        return socket
    end,
    udp = socket_core.udp_bind,
    udp_x = socket_core.udp_listen,
    udp_r = function(...)
        local socket = socket_core.udp_listen(...)
        socket:set_reliable()
        return socket
    end,
    unix = socket_core.unix_bind,
    unix_s = socket_core.unix_listen,
}

local client_factory = {
    tcp = socket_core.tcp_connect,
    tcp_x = function(...)
        local socket = socket_core.tcp_connect(...)
        socket:set_package_mode()
        return socket
    end,
    udp = socket_core.udp_connect,
    udp_x = socket_core.udp_connect,
    udp_r = function(...)
        local socket = socket_core.udp_connect(...)
        socket:set_reliable()
        return socket
    end,
    unix = function(a1, a2)
        local socket = socket_core.unix_bind(a1)
        socket:connect(a2)
        return socket
    end,
    unix_s = socket_core.unix_connect,
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
