#pragma once

#include "remove_const.h"
#include "remove_volatile.h"

namespace my_std {

template<typename T>
struct RemoveCV {
    using type = remove_const_t<remove_volatile_t<T>>;
};

template<typename T>
using remove_cv_t = typename RemoveCV<T>::type;

} // namespace my_std