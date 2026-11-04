#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <stdlib.h>  // setenv

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

std::vector<TestCase> tests;

std::vector<TestCase>& Registry() {
    return tests;
}

void RegisterTest(std::string name, std::function<void()> fn) {
    Registry().push_back(TestCase{std::move(name), std::move(fn)});
}

int main() {
    setenv("YANDEX_API_KEY", "test", 1);
    std::size_t passed = 0;
    std::size_t failed = 0;

    for (const auto& tc : Registry()) {
        try {
            tc.fn();
            ++passed;
        }
        catch (const std::exception& e) {
            ++failed;
            std::cerr << "[FAIL] " << tc.name << ": " << e.what() << '\n';
        }
        catch (...) {
            ++failed;
            std::cerr << "[FAIL] " << tc.name << ": unknown exception\n";
        }
    }

    if (failed != 0) {
        std::cerr << "Failed: " << failed << ", passed: " << passed << '\n';
        return 1;
    }
    std::cout << "All tests passed: " << passed << '\n';
    return 0;
}
