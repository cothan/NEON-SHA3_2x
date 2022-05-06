#include <benchmark/benchmark.h>

/* 
 * This benchmark code is modified from: https://github.com/bwesterb/armed-keccak/blob/main/benchmark.cxx
 */

#include "fips202.h"
#include "fips202x2.h"


static void BM_F1600x2(benchmark::State& state) {
    v128 a[25] = {0};
    for (auto _ : state) {
        KeccakF1600_StatePermutex2(&a[0]);
        benchmark::DoNotOptimize(a);
    }
}

static void BM_F1600(benchmark::State& state) {
    uint64_t a[25] = {0};
    for (auto _ : state) {
        KeccakF1600_StatePermute(a);
        benchmark::DoNotOptimize(a);
    }
}

BENCHMARK(BM_F1600x2);
BENCHMARK(BM_F1600);
BENCHMARK_MAIN();
