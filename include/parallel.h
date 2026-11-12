#pragma once
#include <optional>

#include "matrix.h"

namespace sem7 {

// Результат одного запуска умножения.
struct RunResult {
    Matrix product;  // матрица-результат A = B * C
    double ms = 0.0;  // измеренное время, миллисекунды
    unsigned threads = 1;  // сколько потоков реально работало
};

// Количество потоков устройства (std::thread::hardware_concurrency()).
// Возвращает 1, если система не сообщает это число.
unsigned hardware_threads();

// Этап 1: однопоточное умножение с таймером high_resolution_clock.
// Возвращает std::nullopt при некорректных размерах матриц.
std::optional<RunResult> run_single(const Matrix& b, const Matrix& c, int n,
                                    LoopOrder order);

// Этап 2: многопоточное умножение (i, k, j), «наивный» замер -
// таймер включает создание потоков и join().
// Возвращает std::nullopt при некорректных данных или threads == 0.
std::optional<RunResult> run_threads_naive(const Matrix& b, const Matrix& c,
                                           int n, unsigned threads);

// Этапы 3, 5, 6: потоки создаются заранее и ждут атомарный флаг
// (std::atomic<bool> + this_thread::yield()); замеряется только время
// вычислений от установки флага до последнего join().
// Возвращает std::nullopt при некорректных данных или threads == 0.
std::optional<RunResult> run_threads_flag(const Matrix& b, const Matrix& c,
                                          int n, unsigned threads,
                                          LoopOrder order);

// Замеряет время создания и join() пустых потоков (накладные расходы).
// Возвращает std::nullopt, если threads == 0.
std::optional<double> measure_thread_overhead(unsigned threads);

}  // namespace sem7
