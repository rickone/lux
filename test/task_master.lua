require "lux"

local tm = lux_core.create_task_master(20, "task.lua", "Hello", "World")

function tm:on_respond(pt)
	print(pt:unpack())
end

for i = 1, 10 do
	for j = i + 1, 10 do
		tm:request("foo", i, j)
	end
end
