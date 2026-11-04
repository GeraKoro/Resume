#pragma once
#include <functional>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "processing.h"

namespace join_realisation {
template <typename T>
using removeCvRefT = std::remove_cvref_t<T>;
template <typename Container>
using RangeValueT = removeCvRefT<decltype(*std::begin(std::declval<Container&>()))>;
} // namespace join_realisation

template <typename RightContainer>
class JoinKVRealisation {

private:
	RightContainer right_container_;
    
public:
	explicit JoinKVRealisation(RightContainer right_container) : right_container_(std::move(right_container)) {}

    template <typename LeftRange>
    auto operator()(LeftRange&& left_range) const {
		using LeftItem = join_realisation::RangeValueT<LeftRange>;
		using RightItem = join_realisation::RangeValueT<RightContainer>;
		using KeyType = join_realisation::removeCvRefT<decltype(std::declval<RightItem>().key)>;
		using LeftValueType = join_realisation::removeCvRefT<decltype(std::declval<LeftItem>().value)>;
		using RightValueType = join_realisation::removeCvRefT<decltype(std::declval<RightItem>().value)>;

        std::unordered_map<KeyType, RightValueType> right_map;
		for (const RightItem& item : right_container_) {
            right_map[item.key] = item.value;
        }

        std::vector<JoinResult<LeftValueType, RightValueType>> result;
        for (const LeftItem& item : left_range) {
            auto found = right_map.find(item.key);
            if (found == right_map.end()) {
                result.push_back(JoinResult<LeftValueType, RightValueType>{item.value, std::nullopt});
            } 
            else {
                result.push_back(JoinResult<LeftValueType, RightValueType>{item.value, found->second});
            }
        }
        return result;
    }
};

template <typename RightContainer>
auto Join(RightContainer right_container) {
	return JoinKVRealisation<join_realisation::removeCvRefT<RightContainer>>(std::forward<RightContainer>(right_container));
}

template <typename RightContainer, typename LeftFunc, typename RightFunc>
class JoinByKeyRealisation {

private:
	RightContainer right_container_;
    LeftFunc left_func_;
    RightFunc right_func_;

public:
	JoinByKeyRealisation(RightContainer right_container, LeftFunc left_func, RightFunc right_func)
		: right_container_(std::move(right_container)), left_func_(std::move(left_func)), right_func_(std::move(right_func)) {}

    template <typename LeftRange>
    auto operator()(LeftRange&& left_range) const {
		using LeftItem = join_realisation::RangeValueT<LeftRange>;
		using RightItem = join_realisation::RangeValueT<RightContainer>;
		using KeyType = join_realisation::removeCvRefT<decltype(std::invoke(right_func_, std::declval<const RightItem&>()))>;

        std::unordered_map<KeyType, RightItem> right_map;
		for (const RightItem& item : right_container_) {
            right_map[std::invoke(right_func_, item)] = item;
        }

        std::vector<JoinResult<LeftItem, RightItem>> result;
        for (const LeftItem& item : left_range) {
            KeyType key = std::invoke(left_func_, item);
            auto found = right_map.find(key);
            if (found == right_map.end()) {
                result.push_back(JoinResult<LeftItem, RightItem>{item, std::nullopt});
            } 
            else {
                result.push_back(JoinResult<LeftItem, RightItem>{item, found->second});
            }
        }
        return result;
    }
};

template <typename RightContainer, typename LeftFunc, typename RightFunc>
auto Join(RightContainer right_container, LeftFunc left_func, RightFunc right_func) {
	using StoredRightContainer = join_realisation::removeCvRefT<RightContainer>;
	return JoinByKeyRealisation<StoredRightContainer, LeftFunc, RightFunc>(
		std::forward<RightContainer>(right_container), std::move(left_func), std::move(right_func));
}