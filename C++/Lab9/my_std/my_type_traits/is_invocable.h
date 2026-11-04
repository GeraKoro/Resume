#pragma once

#include "true_false_type.h"
#include "../my_functional/invoke.h"
#include "../my_utility/declval.h"

namespace my_std {

template<typename, typename = void, typename...>
struct IsInvocable : false_type {};

template<typename F, typename... Args>
struct IsInvocable<F, decltype(void(my_std::Invoke(my_std::Declval<F>(), my_std::Declval<Args>()...))), Args...>
    : true_type {};

template<typename F, typename... Args>
inline constexpr bool is_invocable_v = IsInvocable<F, void, Args...>::value;

} //namespace my_std