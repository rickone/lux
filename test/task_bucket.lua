require "lux"

local tb = lux_core.create_task_bucket("task.lua", 50)

function tb:on_respond(pt)
	print(pt:unpack())
end

for i = 1, 100 do
	for j = i + 1, 100 do
		tb:request("foo", i, j)
	end
end
