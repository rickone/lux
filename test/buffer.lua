local b = buffer.new()
b.min_alloc_size = 8

b:push_string("Hello")
print(b:dump(), b, b:max_size())

print(b:pop_string(3))

b:push_string(" World")
print(b:dump(), b, b:max_size())

b:push_string(" This is a buffer test")
print(b:dump(), b, b:max_size())