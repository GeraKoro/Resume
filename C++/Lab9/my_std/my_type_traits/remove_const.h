#pragma once

namespace my_std {


template<typename T>
struct RemoveConst {
    using type = T;
};

template<typename T>
struct RemoveConst<const T> {
    using type = T;
};

template<typename T>
using remove_const_t = typename RemoveConst<T>::type;

} // namespace my_std