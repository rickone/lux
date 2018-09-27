require "lux"

local t1 = lux_core.create_timer(50, 20, 0)
t1.on_timer = function()
    print(1, t1.counter)
end

local t2 = lux_core.create_timer(100, 10, 0)
t2.on_timer = function()
    print(2, t2.counter)
end
