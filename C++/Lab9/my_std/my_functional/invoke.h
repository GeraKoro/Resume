#pragma once

#include "../my_type_traits/type_traits.h"
#include "../my_utility/forward.h"


namespace my_std {

template<typename T>
struct IsRawPointer : false_type {};

template<typename T>
struct IsRawPointer<T*> : true_type {};

template<typename Func, typename Class, typename... Args>
decltype(auto) InvokeImpl(Func&& func, Class&& clas, Args&&... args) {
    using ClassDecay = remove_cvref_t<Class>;
    if constexpr (IsRawPointer<ClassDecay>::value) {
        return ((*Forward<Class>(clas)).*func)(Forward<Args>(args)...);
    }
    else {
        return (Forward<Class>(clas).*func)(Forward<Args>(args)...);
    }
}

template<typename Func, typename... Args>
decltype(auto) Invoke(Func&& func, Args&&... args) {
    if constexpr (is_member_function_pointer_v<remove_cvref_t<Func>>) {
        return InvokeImpl(Forward<Func>(func), Forward<Args>(args)...);
    } 
    else {
        return Forward<Func>(func)(Forward<Args>(args)...);
    }
}

} // namespace my_std