#pragma once
#include <cstddef>
#include <optional>
#include <vector>

namespace sem7 {

// Квадратная матрица n x n, хранится построчно в одном векторе:
// элемент (i, j) лежит в позиции i * n + j.
using Matrix = std::vector<double>;

// Порядок вложенных циклов при умножении.
enum class LoopOrder { kIKJ, kIJK };

inline constexpr int kMinSize = 1;  // минимальный размер матрицы
inline constexpr int kMaxSize = 3000;  // максимальный размер матрицы
inline constexpr int kMaxValue = 10;  // элементы заполняются числами 0..9

// Проверяет, что размер n лежит в диапазоне [kMinSize, kMaxSize].
bool is_valid_size(int n);

// Создаёт матрицу n x n со случайными целыми значениями 0..9.
// seed - зерно генератора (одинаковый seed - одинаковая матрица).
// Возвращает std::nullopt, если размер n некорректен.
std::optional<Matrix> make_random_matrix(int n, unsigned seed);

// Умножает строки [row_begin, row_end) матриц: A = B * C (результат
// прибавляется к a). order - порядок циклов (i, k, j) или (i, j, k).
// Возвращает false при некорректных размерах или диапазоне строк.
bool multiply_rows(Matrix& a, const Matrix& b, const Matrix& c, int n,
                   int row_begin, int row_end, LoopOrder order);

// Сравнивает две матрицы поэлементно с точностью eps.
// Возвращает false, если размеры различаются.
bool matrices_equal(const Matrix& x, const Matrix& y, double eps);

}  // namespace sem7
