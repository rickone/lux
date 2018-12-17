require "lux"

local r = lux_core.create_routine()
print("->", r:run(function(a, b)
    print(":", a, b)
    print(":", coroutine.yield(1, 2, 3))
    print(":", coroutine.yield("Hello", "World"))
    return "done" + 3.14
end, 100, "Lux"
))

print("->", r:resume(3, 2, 1))
print("->", r:resume("World", "Hello"))
