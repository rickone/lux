-- task will be executed in sub-thread

function init(a, b)
	return "init-> "..a.." "..b
end

function foo(a, b)
	return ("foo %d + %d = %d"):format(a, b, a + b)
end
