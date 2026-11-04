#pragma once

namespace my_std {

template<typename T>
struct RemoveVolatile {
    using type = T;
};

template<typename T>
struct RemoveVolatile<volatile T> {
    using type = T;
};

template<typename T>
using remove_volatile_t = typename RemoveVolatile<T>::type;

} // namespace my_std