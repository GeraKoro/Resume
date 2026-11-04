#pragma once

#include "../my_cstdef/cstdef.h"
#include "../my_utility/utility.h"
#include "tuple_impl.h"

namespace my_std {

template<size_t I, typename Tuple>
struct TupleElementImpl;

template<size_t I, typename Head, typename... Tail>
struct TupleElementImpl<I, Tuple<Head, Tail...>> : TupleElementImpl<I - 1, Tuple<Tail...>> {};

template<typename Head, typename... Tail>
struct TupleElementImpl<0, Tuple<Head, Tail...>> {
    using type = Head;
};


template<size_t I, typename Tuple> 
struct TupleGetter;

template<size_t I, typename Head, typename... Tail>
struct TupleGetter<I, Tuple<Head, Tail...> > {
    template<typename TupleReference>
    static decltype(auto) Get(TupleReference&& tuple) {
        return TupleGetter<I - 1, Tuple<Tail...>>::Get(tuple.tail);
    }
};

template<typename Head, typename... Tail>
struct TupleGetter<0, Tuple<Head, Tail...> > {
    template<typename TupleReference>
    static decltype(auto) Get(TupleReference&& tuple) {
        return (tuple.head);
    }
};

template<size_t I, typename Tuple>
decltype(auto) Get(Tuple&& tuple) {
    using T = remove_const_t<remove_reference_t<Tuple>>;
    return TupleGetter<I, T>::Get(Forward<Tuple>(tuple));
}

} // namespace my_std