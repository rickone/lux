require "lux"

local tb = lux_core.create_task_bucket(50)

function tb:on_respond(pt)
	print(pt:unpack())
end

for i = 1, 100 do
	for j = i + 1, 100 do
		tb:request(i, j)
	end
end
