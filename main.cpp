#include <iostream>
#include <array>

int main() {
    std::cout << "Hello, world!\n";

    // Initializăm vectorul cu 0 pentru a evita erorile MSan
    std::array<int, 100> v{};
    for (int i = 0; i < 100; ++i) {
        v[i] = 0;
    }

    // Inițializare variabilă nr
    int nr = 0;
    std::cout << "Introduceți nr (max 100): ";

    // Citire și validare input
    if (!(std::cin >> nr) || nr < 0 || nr > 100) {
        std::cerr << "Număr invalid!\n";
        return 1;
    }

    /////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < nr; ++i) {
        std::cout << "v[" << i << "] = ";
        if (!(std::cin >> v[i])) {
            std::cerr << "Input invalid pentru v[" << i << "]!\n";
            return 1;
        }
    }

    std::cout << "\n\n";
    std::cout << "Am citit de la tastatură " << nr << " elemente:\n";
    for (int i = 0; i < nr; ++i) {
        std::cout << "- " << v[i] << "\n";
    }

    return 0;
}
