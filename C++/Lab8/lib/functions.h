#pragma once

#include <functional>
#include <type_traits>
#include <utility>

template<typename Container, typename Func>
struct TransformView {
    std::remove_cvref_t<Container> container;
    Func func;

    struct Iterator {
        decltype(std::declval<Container>().begin()) it;
        decltype(std::declval<Container>().end()) end;
        Func* func;

        Iterator(decltype(container.begin()) it, decltype(container.end()) end, Func* func) : it(it), end(end), func(func) {}

        auto operator*() const { return std::invoke(*func, *it); }
        Iterator& operator++() { ++it; return *this; }
        bool operator!=(const Iterator& other) const { return it != other.it; }
    };

    //NOLINTNEXTLINE
    auto begin() { return Iterator{container.begin(), container.end(), &func}; }
    //NOLINTNEXTLINE
    auto end() { return Iterator{container.end(), container.end(), &func}; }
};

template<typename Func>
struct TransformAdapter {
    Func func;

    template<typename Container>
    auto operator()(Container&& container) const {
        return TransformView<Container, Func>{std::forward<Container>(container), func};
    }
};



template<typename Func>
auto Transform(Func func) { return TransformAdapter<Func>{func}; }




template<typename Container, typename Condition>
struct FilterView {
    std::remove_cvref_t<Container> container;
    Condition condition;

    struct Iterator {
        decltype(std::declval<Container>().begin()) it;
        decltype(std::declval<Container>().end()) end;
        Condition* condition;

        Iterator(decltype(container.begin()) it, decltype(container.end()) end, Condition* condition) : it(it), end(end), condition(condition) { 
            Filt(); 
        }

        void Filt() { while (it != end && !std::invoke(*condition, *it)) { ++it; } }

        decltype(auto) operator*() const { return *it; }
        Iterator& operator++() { ++it; Filt(); return *this; }
        bool operator!=(const Iterator& other) const { return it != other.it; }
    };

    //NOLINTNEXTLINE
    auto begin() { return Iterator{container.begin(), container.end(), &condition}; }
    //NOLINTNEXTLINE
    auto end() { return Iterator{container.end(), container.end(), &condition}; }
};


template<typename Condition>
struct FilterAdapter {
    Condition condition;

    template<typename Container>
    auto operator()(Container&& container) const {
        return FilterView<Container, Condition>{std::forward<Container>(container), condition};
    }
};



template<typename Condition>
auto Filter(Condition condition) { return FilterAdapter<Condition>{condition}; }