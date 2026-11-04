#pragma once

#include <vector>
#include <type_traits>
#include <utility>
#include <iterator>

struct AsVector {
    template<typename Container>
    auto operator()(Container&& container) {
        using value_type = std::remove_cvref_t<decltype(*std::begin(container))>;
        std::vector<value_type> res;
        for (auto&& item : container) {
            res.push_back(item);
        }
        return res;
    }
};

inline AsVector AsVector() { return {}; }