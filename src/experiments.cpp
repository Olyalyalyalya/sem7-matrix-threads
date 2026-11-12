#include "experiments.h"

#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "matrix.h"
#include "parallel.h"

namespace sem7 {

namespace {

constexpr unsigned kSeedB = 1;  // зерно для матрицы B
constexpr unsigned kSeedC = 2;  // зерно для матрицы C
constexpr double kEps = 1e-9;  // точность сравнения результатов
constexpr int kTableSizes[] = {300, 500, 800, 1500};

// Дополняет строку пробелами до width символов. Русские буквы в UTF-8
// занимают 2 байта, поэтому std::setw для них не подходит: считаем
// символы, пропуская байты-продолжения вида 10xxxxxx.
std::string pad(const std::string& text, std::size_t width, bool right) {
    std::size_t chars = 0;
    for (unsigned char ch : text) {
        if ((ch & 0xC0) != 0x80) {
            ++chars;
        }
    }
    const std::string spaces(chars < width ? width - chars : 0, ' ');
    return right ? spaces + text : text + spaces;
}

// Число с двумя знаками после точки в виде строки.
std::string fmt(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value;
    return out.str();
}

// Вид запуска.
enum class Variant { kSingle, kNaive, kFlag };

// Входные матрицы B и C одного размера.
struct Inputs {
    Matrix b;
    Matrix c;
};

// Создаёт входные матрицы. std::nullopt - если n некорректен.
std::optional<Inputs> make_inputs(int n) {
    auto b = make_random_matrix(n, kSeedB);
    auto c = make_random_matrix(n, kSeedC);
    if (!b || !c) {
        std::cout << "Ошибка: некорректный размер матрицы n = " << n << "\n";
        return std::nullopt;
    }
    return Inputs{*b, *c};
}

// Один запуск выбранного варианта.
std::optional<RunResult> run_once(const Inputs& in, int n, Variant v,
                                  unsigned threads, LoopOrder order) {
    if (v == Variant::kSingle) {
        return run_single(in.b, in.c, n, order);
    }
    if (v == Variant::kNaive) {
        return run_threads_naive(in.b, in.c, n, threads);
    }
    return run_threads_flag(in.b, in.c, n, threads, order);
}

// Среднее время из kRepeats запусков (после одного прогревочного);
// результат последнего запуска
// сохраняется для проверки правильности.
std::optional<RunResult> measure(const Inputs& in, int n, Variant v,
                                 unsigned threads, LoopOrder order) {
    // прогревочный запуск не учитывается: первый запуск медленнее из-за
    // выделения памяти и загрузки данных в кэш
    if (!run_once(in, n, v, threads, order)) {
        return std::nullopt;
    }
    std::optional<RunResult> last;
    double total = 0.0;
    for (int r = 0; r < kRepeats; ++r) {
        last = run_once(in, n, v, threads, order);
        if (!last) {
            return std::nullopt;
        }
        total += last->ms;
    }
    last->ms = total / kRepeats;
    return last;
}

// Печать строки результата.
void print_run(const std::string& name, const RunResult& r, int n) {
    std::cout << pad(name, 26, false) << ": потоков = " << r.threads
              << ", n = " << n << ", время = " << fmt(r.ms) << " мс\n";
}

// Печать ускорения base / current.
void print_speedup(const std::string& what, double base, double current) {
    std::cout << what << ": " << fmt(base / current) << "x\n";
}

// Проверка, что результат совпал с эталоном; печатает итог проверки.
bool check_same(const RunResult& reference, const RunResult& r) {
    const bool same = matrices_equal(reference.product, r.product, kEps);
    std::cout << "Проверка результата: "
              << (same ? "матрицы совпадают с однопоточной"
                       : "ОШИБКА - матрицы не совпадают")
              << "\n";
    return same;
}

// Печать заголовка этапа.
void print_title(const std::string& title, int n) {
    std::cout << "=== " << title << " (n = " << n << ", повторов = " << kRepeats
              << ", потоков устройства = " << hardware_threads() << ") ===\n";
}

// Однопоточный эталон (i, k, j) - нужен на нескольких этапах.
std::optional<RunResult> baseline(const Inputs& in, int n) {
    return measure(in, n, Variant::kSingle, 1, LoopOrder::kIKJ);
}

// Строка итоговой таблицы этапа 6.
void print_table_row(const std::string& name, const RunResult& r, int n,
                     double base_ms) {
    std::cout << pad(name, 46, false) << pad(std::to_string(r.threads), 7, true)
              << pad(std::to_string(n), 8, true) << pad(fmt(r.ms), 11, true)
              << pad(fmt(base_ms / r.ms) + "x", 11, true) << "\n";
}

}  // namespace

bool run_stage1(int n) {
    print_title("Этап 1. Исходный код (i, k, j), один поток", n);
    auto in = make_inputs(n);
    if (!in) return false;
    auto r = baseline(*in, n);
    if (!r) return false;
    print_run("Исходный код (i, k, j)", *r, n);
    return true;
}

bool run_stage2(int n) {
    print_title("Этап 2. Потоки (наивный замер)", n);
    auto in = make_inputs(n);
    if (!in) return false;
    auto base = baseline(*in, n);
    auto naive =
        measure(*in, n, Variant::kNaive, hardware_threads(), LoopOrder::kIKJ);
    if (!base || !naive) return false;
    print_run("1 поток (этап 1)", *base, n);
    print_run("Потоки (наивно)", *naive, n);
    print_speedup("Ускорение относительно этапа 1", base->ms, naive->ms);
    return check_same(*base, *naive);
}

bool run_stage3(int n) {
    print_title("Этап 3. Потоки и правильный замер (атомарный флаг)", n);
    auto in = make_inputs(n);
    if (!in) return false;
    auto base = baseline(*in, n);
    auto naive =
        measure(*in, n, Variant::kNaive, hardware_threads(), LoopOrder::kIKJ);
    auto flag =
        measure(*in, n, Variant::kFlag, hardware_threads(), LoopOrder::kIKJ);
    if (!base || !naive || !flag) return false;
    print_run("Потоки (наивно, этап 2)", *naive, n);
    print_run("Потоки с флагом", *flag, n);
    const auto overhead = measure_thread_overhead(hardware_threads());
    std::cout << std::fixed << std::setprecision(3)
              << "Разница наивного замера и замера с флагом: "
              << naive->ms - flag->ms << " мс\n";
    if (overhead) {
        std::cout << "Создание и join() пустых потоков: " << *overhead
                  << " мс\n";
    }
    print_speedup("Ускорение относительно этапа 1", base->ms, flag->ms);
    return check_same(*base, *flag);
}

bool run_stage4() {
    std::cout << "=== Этап 4. Масштабирование (закон Амдала), повторов = "
              << kRepeats << " ===\n";
    std::cout << pad("n", 6, true) << " | " << pad("1 поток (мс)", 14, true)
              << " | " << pad("4 потока (мс)", 14, true)
              << " | Ускорение (T1 / T4)\n";
    for (int n : kTableSizes) {
        auto in = make_inputs(n);
        if (!in) return false;
        auto one = baseline(*in, n);
        auto four =
            measure(*in, n, Variant::kFlag, kTableThreads, LoopOrder::kIKJ);
        if (!one || !four) return false;
        if (!matrices_equal(one->product, four->product, kEps)) {
            std::cout << "ОШИБКА: результаты не совпали при n = " << n << "\n";
            return false;
        }
        std::cout << pad(std::to_string(n), 6, true) << " | "
                  << pad(fmt(one->ms), 14, true) << " | "
                  << pad(fmt(four->ms), 14, true) << " | "
                  << fmt(one->ms / four->ms) << "x\n";
    }
    std::cout << "Проверка результата: при всех n матрицы совпадают\n";
    return true;
}

bool run_stage5(int n) {
    print_title("Этап 5. Потоки и порядок (i, j, k)", n);
    auto in = make_inputs(n);
    if (!in) return false;
    auto base = baseline(*in, n);
    auto ikj =
        measure(*in, n, Variant::kFlag, hardware_threads(), LoopOrder::kIKJ);
    auto ijk =
        measure(*in, n, Variant::kFlag, hardware_threads(), LoopOrder::kIJK);
    if (!base || !ikj || !ijk) return false;
    print_run("Потоки, порядок (i, k, j)", *ikj, n);
    print_run("Потоки, порядок (i, j, k)", *ijk, n);
    print_speedup("Во сколько раз (i, j, k) медленнее (i, k, j)", ijk->ms,
                  ikj->ms);
    print_speedup("Ускорение (i, j, k) относительно этапа 1", base->ms,
                  ijk->ms);
    return check_same(*base, *ijk);
}

bool run_stage6(int n) {
    print_title("Этап 6. Итоговая таблица", n);
    auto in = make_inputs(n);
    if (!in) return false;
    const unsigned hw = hardware_threads();
    auto base = baseline(*in, n);
    auto naive = measure(*in, n, Variant::kNaive, hw, LoopOrder::kIKJ);
    auto flag = measure(*in, n, Variant::kFlag, hw, LoopOrder::kIKJ);
    auto ijk = measure(*in, n, Variant::kFlag, hw, LoopOrder::kIJK);
    auto final_run = measure(*in, n, Variant::kFlag, hw, LoopOrder::kIKJ);
    if (!base || !naive || !flag || !ijk || !final_run) return false;
    std::cout << pad("Код", 46, false) << pad("Потоки", 7, true)
              << pad("Размер", 8, true) << pad("Время, мс", 11, true)
              << pad("Ускорение", 11, true) << "\n";
    print_table_row("Исходный код (i, k, j)", *base, n, base->ms);
    print_table_row("Потоки (наивно)", *naive, n, base->ms);
    print_table_row("Потоки и правильный замер (с флагом)", *flag, n, base->ms);
    print_table_row("Порядок (i, j, k), потоки, правильный замер", *ijk, n,
                    base->ms);
    print_table_row("Порядок (i, k, j), потоки, правильный замер", *final_run,
                    n, base->ms);
    return check_same(*base, *final_run) && check_same(*base, *ijk);
}

}  // namespace sem7
