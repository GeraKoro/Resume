#pragma once

#include "remove_cv.h"
#include "remove_reference_t.h"

namespace my_std {

template<typename T>
struct RemoveCVRef {
    using type = remove_cv_t<remove_reference_t<T>>;
};

template<typename T>
using remove_cvref_t = RemoveCVRef<T>::type;

} // namespace my_std