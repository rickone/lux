package.path = package.path .. ";../lualib/?.lua"

require "tree"

local conn = TcpConnectStream("127.0.0.1", "63790")
local proto = RespStream():subscribe(conn)
local client = Stream():subscribe(proto)

function client:on_data(obj)
    if obj == nil then
        print("Host connected.")
        return
    end

    print("rsp:", tree(obj))
end

function client:on_error(code, info)
    print("error:", code, info)
end

local commander = Stream():subscribe(Core.stdin)
function commander:on_data(data)
    if data == "quit" then
        os.exit()
    end

    conn:write(data)
end

function commander:on_error(code, info)
    print(code, info)
end
