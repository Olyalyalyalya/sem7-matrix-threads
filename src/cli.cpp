#include "cli.h"

#include <iostream>
#include <string>

#include "matrix.h"

namespace sem7 {

namespace {

// Переводит строку в целое число. Возвращает std::nullopt, если в строке
// есть что-то кроме цифр (и минуса в начале) или число слишком длинное.
std::optional<int> to_int(const std::string& s) {
    constexpr std::size_t kMaxDigits = 6;
    if (s.empty() || s.size() > kMaxDigits) {
        return std::nullopt;
    }
    std::size_t start = (s[0] == '-') ? 1 : 0;
    if (start == s.size()) {
        return std::nullopt;
    }
    for (std::size_t i = start; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') {
            return std::nullopt;
        }
    }
    return std::stoi(s);
}

}  // namespace

void print_usage() {
    std::cout << "Использование: ./sem7 [--stage 1..6] [--n " << kMinSize
              << ".." << kMaxSize << "]\n"
              << "  --stage  номер этапа семинара (по умолчанию " << kMaxStage
              << ")\n"
              << "  --n      размер матриц n x n (по умолчанию " << kDefaultSize
              << ", на этапе 4 размеры заданы)\n";
}

std::optional<Options> parse_options(int argc, const char* const argv[]) {
    Options opt;
    for (int i = 1; i < argc; i += 2) {
        const std::string key = argv[i];
        if (i + 1 >= argc) {
            std::cout << "Ошибка: после " << key << " нужно указать значение\n";
            return std::nullopt;
        }
        const std::optional<int> value = to_int(argv[i + 1]);
        if (key == "--stage") {
            if (!value || *value < kMinStage || *value > kMaxStage) {
                std::cout << "Ошибка: --stage должен быть целым числом от "
                          << kMinStage << " до " << kMaxStage
                          << ", а введено: " << argv[i + 1] << "\n";
                return std::nullopt;
            }
            opt.stage = *value;
        } else if (key == "--n") {
            if (!value || !is_valid_size(*value)) {
                std::cout << "Ошибка: --n должен быть целым числом от "
                          << kMinSize << " до " << kMaxSize
                          << ", а введено: " << argv[i + 1] << "\n";
                return std::nullopt;
            }
            opt.n = *value;
        } else {
            std::cout << "Ошибка: неизвестный параметр " << key << "\n";
            return std::nullopt;
        }
    }
    return opt;
}

}  // namespace sem7
