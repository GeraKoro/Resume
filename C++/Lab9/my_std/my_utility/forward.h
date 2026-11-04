#pragma once

#include "../my_type_traits/type_traits.h"

namespace my_std {


template<typename T>
constexpr T&& Forward(remove_reference_t<T>& t) noexcept {
    return static_cast<T&&>(t);
}

template<typename T>
constexpr T&& Forward(remove_reference_t<T>&& t) noexcept {
    static_assert(!is_lvalue_ref_v<T>, "cannot forward an rvalue as an lvalue");
    return static_cast<T&&>(t);
}


} // namespace my_std