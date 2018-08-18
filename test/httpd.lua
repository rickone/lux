require "lux"
local http = require "http"

local command = {}

function command.GET(httpd, socket, request, respond)
    local content = httpd:load_resource(request.url)
    if not content then
        respond:set_code(404)
        return respond:send(socket)
    end

    respond:set_code(200)
    respond:set_content(content)
    respond:send(socket)
end

local session = {}

function session:start(socket, httpd)
    self.entity:add_component(socket)
    socket.on_recv = {self, "on_socket_recv"}

    self.httpd = httpd
end

function session:on_socket_recv(socket, buffer)
    if self.request == nil then
        self.request = http.Request.new()
    end

    if not self.request:parse(buffer) then
        return
    end

    local request = self.request
    self.request = nil

    --print("request", tree(request))
    print("request", request.url)

    local respond = http.Respond.new(request)
    local func = command[request.method]
    if not func then
        respond:set_code(405)
        respond:send(socket)
        return
    end

    func(self.httpd, socket, request, respond)

    if request.header["CONNECTION"] == "keep-alive" then
        local t = self.entity:add_timer(5000, 1)
        t.on_timer = {self, "on_timeout"}
    else
        self.entity:remove()
    end
end

function session:on_timeout()
    self.entity:remove()
end

local httpd = {}

function httpd:start()
    local socket = socket_core.tcp_listen("0.0.0.0", "8866")
    self.entity:add_component(socket)

    socket.on_accept = {self, "on_socket_accept"}

    self.cache = {}
    self.cache_timeout = config.httpd_cache_timeout or 60
    self.url_root = config.httpd_url_root

    local t = self.entity:add_timer(2000, -1)
    t.on_timer = {self, "update_cache"}
end

function httpd:on_socket_accept(listen_socket, socket)
    local ent = lux_core.create_entity()
    ent:add_component(session, socket, self)
end

function httpd:load_resource(url)
    local obj = self.cache[url]
    if obj then
        obj.timeout = os.time() + self.cache_timeout
        return obj.data
    end

    local url_path = self.url_root .. url
    local f = io.open(url_path, "rb")
    if not f then
        return nil
    end

    local data = f:read("a")
    f:close()
    self.cache[url] = {
        data = data,
        timeout = os.time() + self.cache_timeout,
    }
    print("load cache", url)
    return data
end

function httpd:update_cache()
    local now = os.time()
    local cache = self.cache
    for k,v in pairs(cache) do
        if now >= v.timeout then
            print("remove cache", k)
            cache[k] = nil
        end
    end
end

return httpd
