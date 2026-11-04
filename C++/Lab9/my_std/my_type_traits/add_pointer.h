#pragma once

namespace my_std {

template <typename T>
struct AddPointer {
    using type = T*;
};

template <typename T>
using add_pointer_t = typename AddPointer<T>::type;

} // namespace my_std