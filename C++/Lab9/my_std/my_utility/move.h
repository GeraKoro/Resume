#pragma once

#include "../my_type_traits/type_traits.h"

namespace my_std {

template<typename T>
constexpr my_std::remove_reference_t<T>&& Move(T&& t) noexcept {
    return static_cast<remove_reference_t<T>&&>(t);
}

} // namespace my_std