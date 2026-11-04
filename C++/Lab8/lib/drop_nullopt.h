#pragma once

#include "functions.h"

struct DropNulloptRealisation {
    template <typename Container>
    auto operator()(Container&& container) const {
        return std::forward<Container>(container)
            | Filter([](const auto& value) { return value.has_value(); })
            | Transform([](const auto& value) { return *value; });
    }
};

inline auto DropNullopt() { return DropNulloptRealisation{}; }