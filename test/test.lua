require "lux"

print("测试开始")

for k,v in pairs(_G.class_info) do
    print(k,tree(v))
end

for k,v in pairs(_G.class_meta) do
    print(k,tree(v))
end

print("lux_core", tree(lux_core))
print("socket_core", tree(socket_core))
