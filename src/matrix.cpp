#include "matrix.h"

#include <cmath>
#include <random>

namespace sem7 {

namespace {

// Проверяет, что матрица имеет ровно n * n элементов.
bool has_size(const Matrix& m, int n) {
    return m.size() ==
           static_cast<std::size_t>(n) * static_cast<std::size_t>(n);
}

// Умножение строк в порядке (i, k, j): внутренний цикл идёт по строке
// матриц a и c подряд - это быстро для кэша.
void rows_ikj(Matrix& a, const Matrix& b, const Matrix& c, int n, int row_begin,
              int row_end) {
    for (int i = row_begin; i < row_end; ++i) {
        for (int k = 0; k < n; ++k) {
            const double bik = b[i * n + k];
            for (int j = 0; j < n; ++j) {
                a[i * n + j] += bik * c[k * n + j];
            }
        }
    }
}

// Умножение строк в порядке (i, j, k): матрица c читается по столбцам,
// каждый шаг - прыжок на n элементов, поэтому медленно.
void rows_ijk(Matrix& a, const Matrix& b, const Matrix& c, int n, int row_begin,
              int row_end) {
    for (int i = row_begin; i < row_end; ++i) {
        for (int j = 0; j < n; ++j) {
            double sum = a[i * n + j];
            for (int k = 0; k < n; ++k) {
                sum += b[i * n + k] * c[k * n + j];
            }
            a[i * n + j] = sum;
        }
    }
}

}  // namespace

bool is_valid_size(int n) { return n >= kMinSize && n <= kMaxSize; }

std::optional<Matrix> make_random_matrix(int n, unsigned seed) {
    if (!is_valid_size(n)) {
        return std::nullopt;
    }
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> dist(0, kMaxValue - 1);
    Matrix m(static_cast<std::size_t>(n) * static_cast<std::size_t>(n));
    for (double& x : m) {
        x = dist(gen);
    }
    return m;
}

bool multiply_rows(Matrix& a, const Matrix& b, const Matrix& c, int n,
                   int row_begin, int row_end, LoopOrder order) {
    if (!is_valid_size(n) || !has_size(a, n) || !has_size(b, n) ||
        !has_size(c, n)) {
        return false;
    }
    if (row_begin < 0 || row_end > n || row_begin > row_end) {
        return false;
    }
    if (order == LoopOrder::kIKJ) {
        rows_ikj(a, b, c, n, row_begin, row_end);
    } else {
        rows_ijk(a, b, c, n, row_begin, row_end);
    }
    return true;
}

bool matrices_equal(const Matrix& x, const Matrix& y, double eps) {
    if (x.size() != y.size()) {
        return false;
    }
    for (std::size_t i = 0; i < x.size(); ++i) {
        if (std::fabs(x[i] - y[i]) > eps) {
            return false;
        }
    }
    return true;
}

}  // namespace sem7
