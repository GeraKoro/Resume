#pragma once

namespace my_std {

template<bool Bool>
struct BoolConstant {
    static constexpr bool value = Bool;
};

using true_type = BoolConstant<true>;
using false_type = BoolConstant<false>;

} // namespace my_std