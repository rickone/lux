require "core"

for k,v in pairs(_G.class_metas) do
    print(k,tree(v))
end

