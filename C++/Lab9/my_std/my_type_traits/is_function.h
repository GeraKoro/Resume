#pragma once

#include "true_false_type.h"

namespace my_std {

template <typename>
struct IsFunction : false_type {};

template <typename Func, typename... Args>
struct IsFunction<Func(Args...)> : true_type {};

template<typename T>
inline constexpr bool is_function_v = IsFunction<T>::value;

} // namespace my_std