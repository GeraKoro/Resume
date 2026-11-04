#pragma once

#include "../my_cstdef/cstdef.h"
#include "../my_utility/move.h"

namespace my_std {

template<typename... Args>
struct Tuple {};

template<typename Head, typename... Tail>
struct Tuple<Head, Tail...> {
    Head head;
    Tuple<Tail...> tail;

    Tuple(Head h, Tail... t) : head(Move(h)), tail(Move(t)...) {}
};

template<>
struct Tuple<> {};


template<typename T>
struct TupleSize;

template<typename Head, typename... Tail>
struct TupleSize<Tuple<Head, Tail...>> {
    static constexpr size_t value = 1 + TupleSize<Tuple<Tail...>>::value;
};

template<>
struct TupleSize<Tuple<>> {
    static constexpr size_t value = 0;
};


} //namespace my_std