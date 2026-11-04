#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

namespace fs = std::filesystem;

class DirRealisation {
public:
	DirRealisation(fs::path path, bool recursive) : path_(std::move(path)), recursive_(recursive) {}

    class Iterator {
    public:
        using IteratorVariant = std::variant<fs::directory_iterator, fs::recursive_directory_iterator>;

		Iterator(const fs::path& path, bool recursive, bool as_end) {
			if (recursive) {
				if (as_end) {
					iterator_.template emplace<1>();
				} else {
					iterator_.template emplace<1>(path);
				}
			} else {
				if (as_end) {
					iterator_.template emplace<0>();
				} else {
					iterator_.template emplace<0>(path);
				}
			}
		}

		static Iterator MakeBegin(const fs::path& path, bool recursive) {
			return Iterator(path, recursive, false);
		}

		static Iterator MakeEnd(bool recursive) { return Iterator(fs::path{}, recursive, true); }

		fs::path operator*() const {
			return std::visit([](const auto& it) -> fs::path { return it->path(); }, iterator_);
		}

		Iterator& operator++() {
			std::visit([](auto& it) { ++it; }, iterator_);
			return *this;
		}

		bool operator!=(const Iterator& other) const {
			return std::visit(
				[](const auto& left, const auto& right) -> bool {
					using LeftType = std::decay_t<decltype(left)>;
					using RightType = std::decay_t<decltype(right)>;
					if constexpr (std::is_same_v<LeftType, RightType>) {
						return left != right;
					}
					return true;
				},
				iterator_, other.iterator_);
		}

    private:
        IteratorVariant iterator_;
    };

	//NOLINTNEXTLINE
	auto begin() { return Iterator::MakeBegin(path_, recursive_); }
	//NOLINTNEXTLINE
	auto end() { return Iterator::MakeEnd(recursive_); }

private:
    fs::path path_;
    bool recursive_;
};

inline auto Dir(const fs::path& path, bool recursive = false) {
	return DirRealisation(path, recursive);
}


struct OpenFiles {
	template <typename Container>
	class OpenFilesRealisation {
    public:
		explicit OpenFilesRealisation(Container container) : container_(std::move(container)) {}

        class Iterator {
        public:
			using UpstreamIterator = decltype(std::begin(std::declval<Container&>()));

			Iterator(UpstreamIterator current, UpstreamIterator end, bool is_end = false)
				: current_(current), end_(end), is_end_(is_end) {
				if (!is_end_) {
					OpenCurrent();
				}
			}

			std::ifstream& operator*() const { return *file_; }

			Iterator& operator++() {
				++current_;
				OpenCurrent();
				return *this;
			}

			bool operator!=(const Iterator& other) const {
				if (is_end_ && other.is_end_) {
					return false;
				}
				if (is_end_ != other.is_end_) {
					return true;
				}
				return current_ != other.current_;
			}

        private:
			void OpenCurrent() {
				file_.reset();
				while (current_ != end_) {
					file_ = std::make_shared<std::ifstream>(*current_);
					if (file_->is_open()) {
						return;
					}
					++current_;
				}
				is_end_ = true;
			}

            UpstreamIterator current_;
            UpstreamIterator end_;
            std::shared_ptr<std::ifstream> file_;
            bool is_end_ = true;
        };

		//NOLINTNEXTLINE
		auto begin() { return Iterator(std::begin(container_), std::end(container_)); }
		//NOLINTNEXTLINE
		auto end() { return Iterator(std::end(container_), std::end(container_), true); }

    private:
		Container container_;
    };

	template <typename Container>
	auto operator()(Container&& container) const {
		using Stored = std::remove_cvref_t<Container>;
		return OpenFilesRealisation<Stored>(std::forward<Container>(container));
	}
};