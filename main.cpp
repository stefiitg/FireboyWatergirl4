#include <iostream>
#include <array>

int main() {
    std::cout << "Hello, world!\n";
    std::array<int, 100> v{};
    int nr = 0; // ✅ inițializare sigură

    std::cout << "Introduceți nr: ";
    if (!(std::cin >> nr)) { // ✅ verificăm dacă inputul e valid
        std::cerr << "Eroare: nu s-a putut citi un număr valid!\n";
        return 1;
    }

    if (nr < 0 || nr > 100) { // ✅ validăm și limita
        std::cerr << "Eroare: numărul trebuie să fie între 0 și 100!\n";
        return 1;
    }

    for (int i = 0; i < nr; ++i) {
        std::cout << "v[" << i << "] = ";
        if (!(std::cin >> v[i])) {
            std::cerr << "Eroare la citirea elementului " << i << "!\n";
            return 1;
        }
    }

    std::cout << "\n\nAm citit de la tastatură " << nr << " elemente:\n";
    for (int i = 0; i < nr; ++i) {
        std::cout << "- " << v[i] << "\n";
    }

    return 0;
}
