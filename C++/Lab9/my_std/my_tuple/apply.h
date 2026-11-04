#pragma once

#include "../my_cstdef/cstdef.h"
#include "../my_utility/utility.h"
#include "../my_type_traits/type_traits.h"
#include "../my_tuple/tuple.h"

namespace my_std {

template<typename Func, typename Tuple, size_t... I>
auto ApplyImpl(Func&& func, Tuple&& tuple, IndexSequence<I...>) {
    return Forward<Func>(func)(Get<I>(tuple)...);
}

template<typename Func, typename Tuple>
auto Apply(Func&& func, Tuple&& tuple) {
    using CleanTuple = remove_const_t<remove_reference_t<Tuple>>;
    return ApplyImpl(Forward<Func>(func)
                            , Forward<Tuple>(tuple)
                            , make_index_sequence<TupleSize<CleanTuple>::value>{}
    );
}

} // namespace my_std