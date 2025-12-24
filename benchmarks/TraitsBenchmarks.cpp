#include <benchmark/benchmark.h>
#include "Mira/ETS/Traits.hpp"

using namespace Mira::ETS;

static void BM_Trait_Update(benchmark::State& state) {
    GameEntity entity(1, 0, 0, 1, 1);
    float dt = 0.016f;
    for (auto _ : state) {
        entity.Update(dt);
    }
}
BENCHMARK(BM_Trait_Update)->Repetitions(3)->ReportAggregatesOnly(false);

static void BM_Trait_GetStatusString(benchmark::State& state) {
    GameEntity entity(1, 10.5f, 20.7f, 1, 1);
    for (auto _ : state) {
        benchmark::DoNotOptimize(entity.GetStatusString());
    }
}
BENCHMARK(BM_Trait_GetStatusString)->Repetitions(3)->ReportAggregatesOnly(false);