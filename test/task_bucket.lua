require "lux"

local tb = lux_core.create_task_bucket(20, "task.lua", "Hello", "World")

function tb:on_respond(pt)
	print(pt:unpack())
end

for i = 1, 10 do
	for j = i + 1, 10 do
		tb:request("foo", i, j)
	end
end
