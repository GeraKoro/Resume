#pragma once

template <typename Container>
struct DataFlow {
    using value_type = typename Container::value_type;

    Container* container;

    //NOLINTNEXTLINE
    auto begin() { return container->begin(); }
    //NOLINTNEXTLINE
    auto end() { return container->end(); }
    //NOLINTNEXTLINE
    auto begin() const { return container->begin(); }
    //NOLINTNEXTLINE
    auto end() const { return container->end(); }
};

template <typename Container>
auto AsDataFlow(Container& container) {
    return DataFlow<Container>{&container};
}