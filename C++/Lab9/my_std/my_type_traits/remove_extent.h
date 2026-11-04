#pragma once

#include "../my_cstdef/cstdef.h"

namespace my_std {

template <typename T>
struct RemoveExtent {
    using type = T;
};

template <typename T>
struct RemoveExtent<T[]> {
    using type = T;
};

template <typename T, size_t N>
struct RemoveExtent<T[N]> {
    using type = T;
};

template <typename T>
using remove_extent_t = typename RemoveExtent<T>::type;

} //namespace my_std