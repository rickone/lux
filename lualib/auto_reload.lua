local auto_reload = {}

local require = require
local inst_timer

function auto_reload.open()
    assert(inst_timer == nil)

    local path2name = {}
    local timer = Timer(3000, -1)
    local monitor = FileMonitor()

    local file_update = function(file_path)
        local name = path2name[file_path]
        print("auto reload:", file_path, name)
        reload(name)
    end

    timer:add_subscriber(monitor)
    monitor:set_listener(msg_type.modify_file, file_update)

    hook(_G, "require", function(name)
        local mod = package.loaded[name]
        if mod ~= nil then
            return mod
        end

        local path = name:gsub("%.", "/")
        for pattern in package.path:gmatch("[^;]+") do
            local file_path = pattern:gsub("%?", path)
            if stat.isreg(file_path) then
                path2name[file_path] = name
                monitor:add(file_path)
                print("monitor:", file_path)
                break
            end
        end

        return require(name)
    end)

    inst_timer = timer
end

function auto_reload.close()
    assert(inst_timer)

    inst_timer:stop()
    inst_timer = nil
end

if _G.auto_reload == nil then
    rawset(_G, "auto_reload", auto_reload)
    auto_reload.open()
end
