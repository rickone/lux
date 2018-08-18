--require "lux"

local body = {}

function body:start()
    print("body:start", self, type(self))

    local t1 = self.entity:add_timer(50, 20)
    t1.on_timer = {self, "update"}

    local t2 = self.entity:add_timer(100, 10)
    t2.on_timer = {self, "on_timer"}
end

function body:stop()
    print("body:stop")
end

function body:update(t)
    print(t, type(t), t.counter)

    if t.counter == 0 then
        self.entity:remove()
    end
end

function body:on_timer(t)
    print(t, type(t), t.counter)
end

return body
