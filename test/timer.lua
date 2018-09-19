require "lux"

local t1 = lux_core.create_timer(50, 20)
t1.on_timer = function(t)
    print(t, type(t), t.counter)
end

local t2 = lux_core.create_timer(100, 10)
t2.on_timer = function(t)
    print(t, type(t), t.counter)
end
