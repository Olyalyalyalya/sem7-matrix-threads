#pragma once
#include <optional>

namespace sem7 {

inline constexpr int kDefaultSize = 800;  // размер матрицы по умолчанию
inline constexpr int kMinStage = 1;
inline constexpr int kMaxStage = 6;

// Параметры запуска программы.
struct Options {
    int stage = kMaxStage;  // номер этапа семинара 1..6
    int n = kDefaultSize;   // размер матриц
};

// Разбирает параметры командной строки: --stage <1..6> и --n <размер>.
// Возвращает std::nullopt и печатает сообщение об ошибке, если параметр
// неизвестен, не имеет значения или значение некорректно.
std::optional<Options> parse_options(int argc, const char* const argv[]);

// Печатает справку о параметрах запуска.
void print_usage();

}  // namespace sem7
