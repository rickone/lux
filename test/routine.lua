require "lux"

for i = 1, 100 do
    lux_core.create_routine(function()
        --lux_core.create_routine(print, i, "Hello Lux", lux_core.self_name())
        print(i, "Hello Lux", lux_core.self_name())
    end)
end
