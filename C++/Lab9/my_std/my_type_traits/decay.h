#pragma once

#include "remove_cv.h"
#include "remove_reference_t.h"
#include "remove_extent.h"
#include "add_pointer.h"
#include "../my_cstdef/cstdef.h"


namespace my_std {

template <typename T>
struct DecayImpl {
    using type = remove_cv_t<T>;
};

template <typename T>
struct DecayImpl<T[]> {
    using type = remove_extent_t<T>*;
};

template <typename T, size_t N>
struct DecayImpl<T[N]> {
    using type = remove_extent_t<T>*;
};

template <typename Func, typename... Args>
struct DecayImpl<Func(Args...)> {
    using type = Func(*)(Args...);
};

template <typename T>
struct Decay {
    using type = typename DecayImpl<remove_reference_t<T>>::type;
};

template <typename T>
using decay_t = typename Decay<T>::type;

} // namespace my_std