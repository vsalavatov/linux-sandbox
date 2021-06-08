#include <bits/stdc++.h>

int main() {
    const std::size_t mem = 100 * 1024 * 1024;
    std::cout << "gonna eat >= " << mem << " bytes" << std::endl;

    char* data = reinterpret_cast<char*>(malloc(mem));

    for (std::size_t i = 0; i < mem; i++) {
        data[i] = (i * 12412 + 12421) % 337 * 293;
    }

    std::cout << "yummy!" << std::endl;

    return 0;
}

