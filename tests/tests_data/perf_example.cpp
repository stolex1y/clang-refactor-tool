#include <vector>
#include <string>

struct HeavyObject {
    std::string data[1000];  // Тяжёлый объект для имитации копирования
};

void cycle(const std::vector<HeavyObject> &vec) {
    for (const auto obj : vec) {  // Копирование без &
        const auto &data = obj.data[0];
    }
}

int main() {
    std::vector<HeavyObject> vec(100000);  // Большой контейнер
    cycle(vec);
    return 0;
}