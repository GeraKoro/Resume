#pragma once

#include <cstddef>
#include <iostream>
#include <string>


struct Out {
	std::ostream& os;

	Out(std::ostream& os) : os(os) {}

	template<typename Container>
	std::ostream& operator()(Container&& container) const {
		for (const auto& item : container) {
			os << item << '\n';
		}
        return os;
	}
};


template <typename Stream>
struct WriteRealisation {
	Stream& stream;
	char delimiter;

	WriteRealisation(Stream& stream, char delimiter) : stream(stream), delimiter(delimiter) {}

    template <typename Container>
    std::size_t operator()(Container&& container) const {
        std::size_t count = 0;
        for (const auto& value : container) {
            stream << value << delimiter;
            ++count;
        }
        return count;
    }
};

template <typename Stream>
auto Write(Stream& stream, char delimiter) {
	return WriteRealisation<Stream>(stream, delimiter);
}