require "core"
local http = require "http"

local command = {name="command"}

function command:start(httpd, socket)
    self.httpd = httpd
    self.socket = socket
    self:subscribe(100, "invoke")
end

function command:invoke(request)
    local respond = http.Respond.new(self.socket, request)
    local func = self[request.method]
    if not func then
        respond:send(405)
        return
    end

    func(self, request, respond)
end

function command:GET(request, respond)
    local content = self.httpd:load_resource(request.url)
    if not content then
        return respond:send(404)
    end

    respond:send(200, content)
end

local session = {name="session"}

function session:start()
    self:subscribe(msg_type.socket_recv, "on_recv")
end

function session:on_recv(buffer)
    if self.request == nil then
        self.request = http.Request.new()
    end

    if not self.request:parse(buffer) then
        return
    end

    local request = self.request
    self.request = nil

    print("request", request.method, request.url)
    self:publish(100, request)

    if request.header["CONNECTION"] == "keep-alive" then
        self:set_timer("on_timeout", 5000, 1)
    else
        self.entity:remove()
    end
end

function session:on_timeout()
    self.entity:remove()
end

local httpd = {name="httpd"}

function httpd:start()
    local socket = socket_core.tcp_listen("::", "8866")
    self.entity:add_component(socket)
    self.socket = socket

    self:subscribe(msg_type.socket_accept, "on_accept")

    self.cache = {}
    self.cache_timeout = config.httpd_cache_timeout or 60
    self.url_root = config.httpd_url_root

    self:set_timer("update_cache", 2000, -1)
end

function httpd:on_accept(socket)
    local ent = lux_core.create_entity()
    ent:add_component(socket)
    ent:add_component(session)
    ent:add_component(command, self, socket)
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