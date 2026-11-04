#pragma once

#include "true_false_type.h"
#include "../my_cstdef/cstdef.h"

namespace my_std {

template<typename T>
struct IsArray : false_type {};

template<typename T>
struct IsArray<T[]> : true_type {};

template<typename T, size_t N>
struct IsArray<T[N]> : true_type {};

template<typename T>
inline constexpr bool is_array_v = IsArray<T>::value;



} // namespace my_std