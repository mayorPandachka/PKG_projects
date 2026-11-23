// Консольный запуск бенчмарка (без Qt): bench_cli [out.csv]
#include "../src/Benchmark.h"
#include <cstdio>
#include <fstream>
int main(int argc, char **argv)
{
    BenchConfig cfg;
    auto tables = RunBenchmark(cfg, [](int d, int t, const std::string &m) {
        std::fprintf(stderr, "\r[%3d%%] %-60s", 100 * d / t, m.c_str());
        return true;
    });
    std::fprintf(stderr, "\n");
    const std::string csv = BenchToCsv(tables);
    if (argc > 1) { std::ofstream f(argv[1], std::ios::binary); f << csv; }
    else std::fputs(csv.c_str(), stdout);
    return 0;
}
