#pragma once

namespace my_std {

template<typename T>
struct IsValueRef {
    static constexpr bool value = false;
};

template<typename T>
struct IsValueRef<T&&> {
    static constexpr bool value = false;
};

template<typename T>
struct IsValueRef<T&> {
    static constexpr bool value = true;
};

template<typename T>
inline constexpr bool is_value_ref_v = IsValueRef<T>::value;

template<typename T>
inline constexpr bool is_lvalue_ref_v = is_value_ref_v<T>;

} // namespace my_std
