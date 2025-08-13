#include <iostream>
#include "ssl/ssl.hpp"

int main() {
    std::cout << "ssl version: " << ssl::version() << "\n";
    std::cout << (ssl::initialize() ? "init ok" : "init failed") << "\n";
    return 0;
}


