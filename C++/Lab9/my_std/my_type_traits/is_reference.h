#pragma once

#include "true_false_type.h"

namespace my_std {

template<typename T>
struct IsReference : false_type {};

template<typename T>
struct IsReference<T&> : true_type {};

template<typename T>
struct IsReference<T&&> : true_type {};

template<typename T>
inline constexpr bool is_reference_v = IsReference<T>::value;

} // namespace my_std