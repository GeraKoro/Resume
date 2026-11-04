#include "api_work.h"
#include "main_entry.h"

#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {
    return MainEntry(argc, argv, std::cin, std::cout, std::cerr, std::make_shared<CprHttp>());
}
