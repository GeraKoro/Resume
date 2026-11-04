#pragma once

#include <optional>
#include <type_traits>
#include <utility>


template <typename Key, typename Value>
struct KV {
	Key key;
	Value value;

    bool operator==(const KV& other) const = default;
};

template <typename LeftValue, typename RightValue>
struct JoinResult {
	LeftValue left;
	std::optional<RightValue> right;

    bool operator==(const JoinResult& other) const = default;
};


template<typename L, typename R, typename = decltype(std::declval<R>()(std::declval<L>()))>
decltype(auto) operator|(L&& l, R&& r) {
    return std::forward<R>(r)(std::forward<L>(l));
}
