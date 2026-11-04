#pragma once

#include "true_false_type.h"

namespace my_std {

template<typename A, typename B>
struct IsSame : false_type {};

template<typename A>
struct IsSame<A, A> : true_type {};

template<typename A, typename B>
inline constexpr bool is_same_v = IsSame<A, B>::value;


} // namespace my_std