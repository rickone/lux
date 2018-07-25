require "core"

local body = {}

function body:start()
    print("body:start", self, type(self))

    self:set_timer("update", 50, 20)
    self:set_timer("on_timer", 100, 10)
end

function body:stop()
    print("body:stop")
end

function body:update(t)
    print(t, type(t), t.counter)

    if t.counter == 0 then
        self:remove_all()
    end
end

function body:on_timer(t)
    print(t, type(t), t.counter)
end

return body
