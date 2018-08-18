require "lux"
local http = require "http"
local http_client = require "http_client"

local host = {}

function host:init(client, url)
    self.client = client
    self.node, self.service = url:match("^(.+):(.+)$")
end

function host:start()
    local socket = socket_core.tcp_connect(self.node, self.service)
    self.entity:add_component(socket)
    self.socket = socket
end

function host:on_socket_connect(socket)
    self.client:on_host_connect()
end

function host:on_socket_recv(socket, buffer)
    self.client:send(tostring(buffer))
    buffer:clear()
end

function host:on_socket_close(socket)
    self.entity:remove()
    self.client.entity:remove()
end

function host:send(data)
    print("host:send", #data)
    self.socket:send(data)
end

local command = {}

function command.GET()
end

function command.CONNECT()
end

local client = {}

function client:init(socket)
    self.socket = socket
end

function client:on_socket_recv(socket, buffer)
    if self.connected then
        self.host:send(tostring(buffer))
        buffer:clear()
        return
    end

    if self.request == nil then
        self.request = http.Request.new()
    end

    if not self.request:parse(buffer) then
        return
    end

    local request = self.request
    self.request = nil

    print("->", tree(request))

    if request.method ~= "CONNECT" then
        --self.entity:remove()
        return
    end

--[[
    if request.header["PROXY-CONNECTION"] == "keep-alive" then
        local t = self.entity:add_timer(5000, 1)
        t.on_timer = {self, "on_timeout"}
    else
        self.entity:remove()
    end
]]

    local ent = lux_core.create_entity()
    self.host = ent:add_component(host, self, request.url)
end

function client:on_socket_close(socket)
    self.entity:remove()
    if self.host then
        self.host.entity:remove()
    end
end

function client:send(data)
    print("client:send", #data)
    self.socket:send(data)
end

function client:on_host_connect()
    self:send("HTTP/1.1 200 Connection Establised\r\n\r\n")
    self.connected = true
end

function client:on_timeout()
    self.entity:remove()
end

local proxy = {}

function proxy:start()
    local socket = socket_core.tcp_listen("::", "8866")
    self.entity:add_component(socket)
end

function proxy:on_socket_accept(listen_socket, socket)
    local ent = lux_core.create_entity()
    ent:add_component(socket)
    ent:add_component(client, socket)
end

return proxy
