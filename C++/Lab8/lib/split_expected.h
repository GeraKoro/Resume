#pragma once
#include <expected>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
class OwnedFlow {
public:
    explicit OwnedFlow(std::vector<T> data) : data_(std::move(data)) {}
    //NOLINTNEXTLINE
    auto begin() { return data_.begin(); }
    //NOLINTNEXTLINE
    auto end() { return data_.end(); }
    //NOLINTNEXTLINE
    auto begin() const { return data_.begin(); }
    //NOLINTNEXTLINE
    auto end() const { return data_.end(); }

private:
    std::vector<T> data_;
};

struct SplitExpectedRealisation {
    template <typename Container>
    auto operator()(Container&& container) const {
        using ExpectedType = std::remove_cvref_t<decltype(*std::begin(container))>;
        using ValueType = typename ExpectedType::value_type;
        using ErrorType = typename ExpectedType::error_type;

        std::vector<ErrorType> unexpected_values;
        std::vector<ValueType> values;

        for (const ExpectedType& value : container) {
            if (value.has_value()) {
                values.push_back(*value);
            } 
            else {
                unexpected_values.push_back(value.error());
            }
        }

        return std::make_pair(OwnedFlow<ErrorType>(std::move(unexpected_values)), OwnedFlow<ValueType>(std::move(values)));
    }
};

inline auto SplitExpected() { return SplitExpectedRealisation{}; }