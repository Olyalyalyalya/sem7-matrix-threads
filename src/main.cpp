#include <iostream>

#include "cli.h"
#include "experiments.h"

// Точка входа: разбирает параметры и запускает нужный этап семинара.
int main(int argc, char* argv[]) {
    const auto options = sem7::parse_options(argc, argv);
    if (!options) {
        sem7::print_usage();
        return 1;
    }
    bool ok = false;
    switch (options->stage) {
        case 1: ok = sem7::run_stage1(options->n); break;
        case 2: ok = sem7::run_stage2(options->n); break;
        case 3: ok = sem7::run_stage3(options->n); break;
        case 4: ok = sem7::run_stage4(); break;
        case 5: ok = sem7::run_stage5(options->n); break;
        default: ok = sem7::run_stage6(options->n); break;
    }
    return ok ? 0 : 1;
}
