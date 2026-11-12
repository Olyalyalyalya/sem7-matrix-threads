#include "parallel.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace sem7 {

namespace {

using Clock = std::chrono::high_resolution_clock;

// Миллисекунды между двумя моментами времени.
double elapsed_ms(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// Проверка входных данных для всех запусков.
bool inputs_ok(const Matrix& b, const Matrix& c, int n) {
    const std::size_t need =
        static_cast<std::size_t>(n) * static_cast<std::size_t>(n);
    return is_valid_size(n) && b.size() == need && c.size() == need;
}

// Первая строка диапазона потока t: строки делятся поровну, остаток
// распределяется по одной строке первым потокам.
int row_start(int n, unsigned threads, unsigned t) {
    const int base = n / static_cast<int>(threads);
    const int extra = n % static_cast<int>(threads);
    const int ti = static_cast<int>(t);
    return ti * base + (ti < extra ? ti : extra);
}

// Потоков не больше, чем строк: лишние потоки остались бы без работы.
unsigned limit_threads(unsigned threads, int n) {
    return threads > static_cast<unsigned>(n) ? static_cast<unsigned>(n)
                                              : threads;
}

}  // namespace

unsigned hardware_threads() {
    const unsigned hw = std::thread::hardware_concurrency();
    return hw == 0 ? 1 : hw;
}

std::optional<RunResult> run_single(const Matrix& b, const Matrix& c, int n,
                                    LoopOrder order) {
    if (!inputs_ok(b, c, n)) {
        return std::nullopt;
    }
    RunResult r;
    r.product.assign(b.size(), 0.0);
    const auto start = Clock::now();
    multiply_rows(r.product, b, c, n, 0, n, order);
    const auto end = Clock::now();
    r.ms = elapsed_ms(start, end);
    r.threads = 1;
    return r;
}

std::optional<RunResult> run_threads_naive(const Matrix& b, const Matrix& c,
                                           int n, unsigned threads) {
    if (!inputs_ok(b, c, n) || threads == 0) {
        return std::nullopt;
    }
    threads = limit_threads(threads, n);
    RunResult r;
    r.product.assign(b.size(), 0.0);
    r.threads = threads;

    const auto start = Clock::now();  // таймер включает создание потоков
    std::vector<std::thread> pool;
    for (unsigned t = 0; t < threads; ++t) {
        const int from = row_start(n, threads, t);
        const int to = row_start(n, threads, t + 1);
        pool.emplace_back(multiply_rows, std::ref(r.product), std::cref(b),
                          std::cref(c), n, from, to, LoopOrder::kIKJ);
    }
    for (std::thread& th : pool) {
        th.join();
    }
    const auto end = Clock::now();
    r.ms = elapsed_ms(start, end);
    return r;
}

std::optional<RunResult> run_threads_flag(const Matrix& b, const Matrix& c,
                                          int n, unsigned threads,
                                          LoopOrder order) {
    if (!inputs_ok(b, c, n) || threads == 0) {
        return std::nullopt;
    }
    threads = limit_threads(threads, n);
    RunResult r;
    r.product.assign(b.size(), 0.0);
    r.threads = threads;

    std::atomic<bool> start_flag(false);
    std::vector<std::thread> pool;
    for (unsigned t = 0; t < threads; ++t) {
        const int from = row_start(n, threads, t);
        const int to = row_start(n, threads, t + 1);
        // поток создаётся заранее и ждёт сигнала, уступая процессор
        pool.emplace_back([&r, &b, &c, &start_flag, n, from, to, order]() {
            while (!start_flag.load()) {
                std::this_thread::yield();
            }
            multiply_rows(r.product, b, c, n, from, to, order);
        });
    }

    const auto start = Clock::now();  // потоки уже созданы
    start_flag.store(true);
    for (std::thread& th : pool) {
        th.join();
    }
    const auto end = Clock::now();
    r.ms = elapsed_ms(start, end);
    return r;
}

std::optional<double> measure_thread_overhead(unsigned threads) {
    if (threads == 0) {
        return std::nullopt;
    }
    const auto start = Clock::now();
    std::vector<std::thread> pool;
    for (unsigned t = 0; t < threads; ++t) {
        pool.emplace_back([]() {});  // поток без работы
    }
    for (std::thread& th : pool) {
        th.join();
    }
    return elapsed_ms(start, Clock::now());
}

}  // namespace sem7
