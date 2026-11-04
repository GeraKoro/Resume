#pragma once

#include "../my_cstdef/cstdef.h"

namespace my_std {

template<size_t... I>
struct IndexSequence {};


template<size_t N, size_t... I>
struct MakeIndexSequenceRealization : MakeIndexSequenceRealization<N - 1, N - 1, I...>{};

template<size_t... I>
struct MakeIndexSequenceRealization<0, I...> {
    using type = IndexSequence<I...>;
};

template<size_t N>
using make_index_sequence = typename MakeIndexSequenceRealization<N>::type;


} // namespace my_std