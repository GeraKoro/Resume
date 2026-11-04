#pragma once

namespace my_std {

template<typename T>
struct RemoveReference {
    using type = T;
};

template<typename T>
struct RemoveReference<T&> {
    using type = T;
}; 

template<typename T>
struct RemoveReference<T&&> {
    using type = T;
}; 

template<typename T>
using remove_reference_t = typename RemoveReference<T>::type;

} // namespace my_std