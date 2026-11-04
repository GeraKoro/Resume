#pragma once

#include "true_false_type.h"

namespace my_std {

template<typename T>
struct IsVoid : false_type {};

template<>
struct IsVoid<void> : true_type {};

template<typename T>
inline constexpr bool is_void_v = IsVoid<T>::value;

} // namespace my_std
