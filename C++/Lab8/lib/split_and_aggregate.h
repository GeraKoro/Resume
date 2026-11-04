//доразобраться

#pragma once

#include <functional>
#include <ios>
#include <istream>
#include <iterator>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace realisation {
template <typename T>
using removeCvRefT = std::remove_cvref_t<T>;
template <typename Container>
using RangeValueT = removeCvRefT<decltype(*std::begin(std::declval<Container&>()))>;
template <typename Container>
using IteratorT = decltype(std::begin(std::declval<Container&>()));
} // namespace realisation

template <typename Container>
class SplitRealisation {

private:
	Container container_;
    std::string delimiters_;
    
public:
	SplitRealisation(Container container, std::string delimiters) : container_(std::move(container)), delimiters_(std::move(delimiters)) {}

    class Iterator {
    public:
		using UpstreamIterator = realisation::IteratorT<Container>;
        using StreamType = realisation::removeCvRefT<decltype(*std::declval<UpstreamIterator>())>;

        Iterator(UpstreamIterator current, UpstreamIterator end, const std::string* delimiters, bool is_end = false)
            : current_(current), end_(end), delimiters_(delimiters), current_stream_(nullptr), is_end_(is_end) {
            if (!is_end_) {
                Advance();
            }
        }

        const std::string& operator*() const { return current_token_; }

        Iterator& operator++() {
            Advance();
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            if (is_end_ && other.is_end_) {
                return false;
            }
            if (is_end_ != other.is_end_) {
                return true;
            }
            if (current_ != other.current_) {
                return true;
            }
            if (current_stream_ != other.current_stream_) {
                return true;
            }
            if (current_stream_ == nullptr) {
                return false;
            }
            return current_stream_->tellg() != other.current_stream_->tellg();
        }

    private:
        UpstreamIterator current_;
        UpstreamIterator end_;
        const std::string* delimiters_;
        StreamType* current_stream_;
        std::string current_token_;
        bool is_end_;
        
        bool IsDelimiter(char c) const { return delimiters_->find(c) != std::string::npos; }

        bool LoadNextSource() {
            while (current_ != end_) {
                StreamType& stream = *current_;
                stream.clear();
                stream.seekg(0, std::ios::beg);
                current_stream_ = &stream;
                return true;
            }
            current_stream_ = nullptr;
            return false;
        }

        void Advance() {
            current_token_.clear();

            while (true) {
                if (current_stream_ == nullptr) {
                    if (!LoadNextSource()) {
                        is_end_ = true;
                        return;
                    }
                }

                char symbol = '\0';
                if (current_stream_->get(symbol)) {
                    if (IsDelimiter(symbol)) {
                        return;
                    }
                    current_token_.push_back(symbol);
                    continue;
                }

                current_stream_->clear();
                current_stream_ = nullptr;
                ++current_;

                if (!current_token_.empty()) {
                    return;
                }
            }
        }
    };

	//NOLINTNEXTLINE
	auto begin() { return Iterator(std::begin(container_), std::end(container_), &delimiters_); }
	//NOLINTNEXTLINE
	auto end() { return Iterator(std::end(container_), std::end(container_), &delimiters_, true); }
};

class SplitRealisationAdapter {
public:
	explicit SplitRealisationAdapter(std::string delimiters) : delimiters_(std::move(delimiters)) {}

	template <typename Container>
	auto operator()(Container&& container) const {
		using Stored = realisation::removeCvRefT<Container>;
		return SplitRealisation<Stored>(std::forward<Container>(container), delimiters_);
	}

private:
    std::string delimiters_;
};

inline auto Split(const std::string& delimiters) { return SplitRealisationAdapter(delimiters); }

template <typename Accum, typename AccumFunc, typename KeyFunc>
class AggregateByKeyRealisation {
public:
	AggregateByKeyRealisation(Accum initial_value, AccumFunc accum_func, KeyFunc key_func)
		: initial_value_(std::move(initial_value)), accum_func_(std::move(accum_func)), key_func_(std::move(key_func)) {}

    template <typename Container>
    auto operator()(Container&& container) const {
		using ValueType = realisation::RangeValueT<Container>;
		using KeyType = realisation::removeCvRefT<decltype(std::invoke(key_func_, std::declval<ValueType>()))>;

        std::unordered_map<KeyType, Accum> agg;
        std::vector<KeyType> key_order;

        for (const auto& value : container) {
            KeyType key = std::invoke(key_func_, value);
            if (agg.find(key) == agg.end()) {
                agg.emplace(key, initial_value_);
                key_order.push_back(key);
            }
            std::invoke(accum_func_, value, agg[key]);
        }

        std::vector<std::pair<KeyType, Accum>> result;
        result.reserve(key_order.size());
        for (const auto& key : key_order) {
            result.push_back(std::make_pair(key, agg[key]));
        }
        return result;
    }

private:
    Accum initial_value_;
    AccumFunc accum_func_;
    KeyFunc key_func_;
};

template <typename Accum, typename AccumFunc, typename KeyFunc>
auto AggregateByKey(Accum initial_value, AccumFunc accum_func, KeyFunc key_func) {
	return AggregateByKeyRealisation<Accum, AccumFunc, KeyFunc>(std::move(initial_value), std::move(accum_func), std::move(key_func));
}