cmake_minimum_required(VERSION 3.16)
project(sem7_matrix_threads LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# По умолчанию собираем с оптимизацией (-O2), иначе замеры бессмысленны
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

find_package(Threads REQUIRED)

# Общая библиотека: всё, кроме main
add_library(sem7_lib
  src/matrix.cpp
  src/parallel.cpp
  src/cli.cpp
  src/experiments.cpp
)
target_include_directories(sem7_lib PUBLIC include)
target_compile_options(sem7_lib PUBLIC -Wall -Wextra -Wpedantic -Werror)
target_link_libraries(sem7_lib PUBLIC Threads::Threads)

# Основная программа
add_executable(sem7 src/main.cpp)
target_link_libraries(sem7 PRIVATE sem7_lib)

# Проверочные сценарии
add_executable(manual_tests tests/manual_tests.cpp)
target_link_libraries(manual_tests PRIVATE sem7_lib)

enable_testing()
add_test(NAME manual_tests COMMAND manual_tests)
