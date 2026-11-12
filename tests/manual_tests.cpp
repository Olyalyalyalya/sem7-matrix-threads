// Проверочные сценарии семинара 7. Запуск: ./build/manual_tests
#include <iostream>
#include <string>
#include <vector>

#include "cli.h"
#include "matrix.h"
#include "parallel.h"

namespace {

int g_failed = 0;  // количество проваленных проверок

// Печатает результат одной проверки.
void check(bool condition, const std::string& name) {
    std::cout << (condition ? "[OK]   " : "[FAIL] ") << name << "\n";
    if (!condition) {
        ++g_failed;
    }
}

// Сценарий 1: известный результат 2 x 2.
void test_known_2x2() {
    const sem7::Matrix b = {1, 2, 3, 4};
    const sem7::Matrix c = {5, 6, 7, 8};
    const sem7::Matrix expected = {19, 22, 43, 50};
    auto r = sem7::run_single(b, c, 2, sem7::LoopOrder::kIKJ);
    check(r && sem7::matrices_equal(r->product, expected, 1e-9),
          "2x2: {1,2,3,4} * {5,6,7,8} = {19,22,43,50}");
}

// Сценарий 2: типичный случай - все варианты дают одинаковый результат.
void test_variants_equal() {
    const int n = 64;
    auto b = sem7::make_random_matrix(n, 1);
    auto c = sem7::make_random_matrix(n, 2);
    auto base = sem7::run_single(*b, *c, n, sem7::LoopOrder::kIKJ);
    auto ijk = sem7::run_single(*b, *c, n, sem7::LoopOrder::kIJK);
    auto naive = sem7::run_threads_naive(*b, *c, n, 4);
    auto flag = sem7::run_threads_flag(*b, *c, n, 4, sem7::LoopOrder::kIKJ);
    auto flag_ijk = sem7::run_threads_flag(*b, *c, n, 4, sem7::LoopOrder::kIJK);
    check(
        base && ijk && sem7::matrices_equal(base->product, ijk->product, 1e-9),
        "n=64: порядок (i,j,k) совпадает с (i,k,j)");
    check(naive && sem7::matrices_equal(base->product, naive->product, 1e-9),
          "n=64: потоки (наивно) совпадают с одним потоком");
    check(flag && sem7::matrices_equal(base->product, flag->product, 1e-9),
          "n=64: потоки с флагом совпадают с одним потоком");
    check(flag_ijk &&
              sem7::matrices_equal(base->product, flag_ijk->product, 1e-9),
          "n=64: потоки (i,j,k) совпадают с одним потоком");
}

// Сценарий 3: граничный случай n = 1 и потоков больше, чем строк.
void test_boundaries() {
    auto one = sem7::run_single({3}, {4}, 1, sem7::LoopOrder::kIKJ);
    check(one && one->product.size() == 1 && one->product[0] == 12,
          "n=1: {3} * {4} = {12}");
    auto b = sem7::make_random_matrix(3, 1);
    auto c = sem7::make_random_matrix(3, 2);
    auto r = sem7::run_threads_flag(*b, *c, 3, 8, sem7::LoopOrder::kIKJ);
    check(r && r->threads == 3, "n=3, 8 потоков: работает 3 потока");
}

// Сценарий 4: некорректные данные не приводят к сбою.
void test_invalid() {
    check(!sem7::make_random_matrix(0, 1), "n=0: матрица не создаётся");
    check(!sem7::make_random_matrix(-5, 1), "n=-5: матрица не создаётся");
    check(!sem7::run_single({}, {}, 2, sem7::LoopOrder::kIKJ),
          "пустые матрицы: std::nullopt");
    check(!sem7::run_single({1, 2, 3, 4}, {1, 2}, 2, sem7::LoopOrder::kIKJ),
          "разные размеры матриц: std::nullopt");
    check(!sem7::run_threads_flag({1}, {1}, 1, 0, sem7::LoopOrder::kIKJ),
          "0 потоков: std::nullopt");
    sem7::Matrix a(4, 0.0);
    check(!sem7::multiply_rows(a, {1, 2, 3, 4}, {1, 2, 3, 4}, 2, 0, 5,
                               sem7::LoopOrder::kIKJ),
          "диапазон строк за пределами матрицы: false");
}

// Сценарий 5: разбор параметров командной строки.
bool parse(const std::vector<const char*>& args) {
    return sem7::parse_options(static_cast<int>(args.size()), args.data())
        .has_value();
}

void test_cli() {
    check(parse({"sem7", "--stage", "3", "--n", "500"}),
          "--stage 3 --n 500: принято");
    check(!parse({"sem7", "--n", "abc"}), "--n abc: отклонено");
    check(!parse({"sem7", "--n", "0"}), "--n 0: отклонено");
    check(!parse({"sem7", "--stage", "9"}), "--stage 9: отклонено");
    check(!parse({"sem7", "--speed", "1"}), "--speed 1: отклонено");
    check(!parse({"sem7", "--n"}), "--n без значения: отклонено");
}

}  // namespace

int main() {
    std::cout << "=== Проверочные сценарии семинара 7 ===\n";
    test_known_2x2();
    test_variants_equal();
    test_boundaries();
    test_invalid();
    test_cli();
    std::cout << (g_failed == 0 ? "Все проверки пройдены\n"
                                : "Есть проваленные проверки: ")
              << (g_failed == 0 ? "" : std::to_string(g_failed) + "\n");
    return g_failed == 0 ? 0 : 1;
}
