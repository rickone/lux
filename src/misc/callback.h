#pragma once

#include "lua_port.h"

template<typename... A>
class Callback
{
public:
    Callback() = default;
    ~Callback() = default;

    void clear()
    {
        _object.reset();
        _func = nullptr;
    }

    template<typename T>
    void set(T *object, const std::function<void (T *, A...)> &func)
    {
        static_assert(std::is_base_of<LuaObject, T>::value, "Callback<T> failed, T must based on LuaObject");

        _object = object->shared_from_this();
        _func = [func](LuaObject *object, A...args){
            func((T *)object, args...);
        };
    }

    template<typename T>
    void set(T *object, void (T::*func)(A...))
    {
        std::function<void (T *, A...)> mfn = std::mem_fn(func);
        set(object, mfn);
    }

    void operator()(A... args)
    {
        auto object = _object.lock();
        if (!object)
            return;

        _func(object.get(), args...);
    }

private:
    std::weak_ptr<LuaObject> _object;
    std::function<void (LuaObject *, A...)> _func;
};
