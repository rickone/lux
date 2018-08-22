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
    socket.on_recv = bind(self.on_socket_recv, self)

    self.httpd = httpd

    local t = self.entity:add_timer(10000, 1)
    t.on_timer = bind(self.entity.remove, self.entity)
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

    if request.header["CONNECTION"] ~= "keep-alive" then
        self.entity:remove()
    end
end

local httpd = {}

function httpd:start()
    local socket = socket_core.tcp_listen("0.0.0.0", "8866")
    self.entity:add_component(socket)

    socket.on_accept = bind(self.on_socket_accept, self)

    self.cache = {}
    self.cache_timeout = config.httpd_cache_timeout or 60
    self.url_root = config.httpd_url_root

    local t = self.entity:add_timer(2000, -1)
    t.on_timer = bind(self.update_cache, self)
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
