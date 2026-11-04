#pragma once

namespace my_std {

template<typename T>
struct IsMemberFunctionPointer : my_std::false_type {};

template<typename Res, typename Class, typename... Args>
struct IsMemberFunctionPointer<Res(Class::*)(Args...)> : my_std::true_type {};

template<typename Res, typename Class, typename... Args>
struct IsMemberFunctionPointer<Res (Class::*)(Args...) const> : my_std::true_type {};

template<typename Res, typename Class, typename... Args>
struct IsMemberFunctionPointer<Res (Class::*)(Args...) volatile> : my_std::true_type {};

template<typename Res, typename Class, typename... Args>
struct IsMemberFunctionPointer<Res (Class::*)(Args...) const volatile> : my_std::true_type {};

template<typename T>
inline constexpr bool is_member_function_pointer_v = IsMemberFunctionPointer<T>::value;

} // namespace my_std