local http = {}

local Request = {}
local RequestMt = {__index = Request}

function Request.new()
    local obj = {}
    obj.parse = Request.parse_cmd

    return setmetatable(obj, RequestMt)
end

function Request:parse_cmd(buffer)
    local i = buffer:find("\r\n")
    if i < 0 then
        return false
    end

    local s = buffer:pop_string(i + 2)
    self.method, self.url, self.version = s:match("^([^%s]+) ([^%s]+) ([^%s]+)\r\n$")

    self.parse = self.parse_header
    self.header = {}

    return self:parse(buffer)
end

function Request:parse_header(buffer)
    local i, j, empty_line = 1, 1, false
    while true do
        local i = buffer:find("\r\n")
        if i < 0 then
            return false
        end

        local s = buffer:pop_string(i + 2)
        if s == "\r\n" then
            break
        end

        local key, value = s:match("^([^:]+):%s*(.-)%s*\r\n$")
        self.header[key:upper()] = value
    end

    if self.method ~= "POST" then
        return true
    end

    self.parse = self.parse_content
    self.content = ""

    return self:parse(buffer)
end

function Request:parse_content(buffer)
    self.content = self.content .. tostring(buffer)
    buffer:clear()

    if #self.content < self.header["CONTENT-LENGTH"] then
        return false
    end

    return true
end

local Respond = {}
local RespondMt = {__index = Respond}

local CodeDefine =
{
    [200] = "OK",
    [404] = "Not Found",
    [405] = "Method Not Allowed",
}

local MimeType =
{
    html = "text/html",
    css = "text/css",
    js = "application/x-javascript",
    png = "image/png",
    jpg = "image/jpeg",
}

function Respond.new(socket, request)
    local obj = {}
    obj.socket = socket
    obj.request = request
    obj.header = {}

    return setmetatable(obj, RespondMt)
end

function Respond:set_header(key, value)
    self.header[key] = value
end

function Respond:send(code, content)
    local result = {}
    table.insert(result, ("%s %s %s"):format(self.request.version, code, CodeDefine[code] or "Unknown"))

    if content and #content > 0 then
        local ext = self.request.url:match("%.(.-)$")
        local mime_type = MimeType[ext]
        if mime_type then
            self:set_header("Content-Type", mime_type)
        end
        self:set_header("Content-Length", #content)
    end
    for k,v in pairs(self.header) do
        table.insert(result, ("%s: %s"):format(k, tostring(v)))
    end
    table.insert(result, "")

    if content then
        table.insert(result, content)
    end

    self.socket:send(table.concat(result, "\r\n"))
    print("respond", code, content and #content)
end

http.Request = Request
http.Respond = Respond

return http