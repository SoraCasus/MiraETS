#include <benchmark/benchmark.h>
#include "Mira/ETS/Movable.hpp"
#include <vector>

using namespace Mira::ETS;

struct SimpleMovable {
    float x = 0;
    void UpdatePosition(float dt) { x += dt; }
};

struct LargeMovable {
    float x = 0;
    std::byte padding[128]; // Force heap allocation (k_BufferSize is 64)
    void UpdatePosition(float dt) { x += dt; }
};

static void BM_AnyMovable_Update_Small(benchmark::State& state) {
    AnyMovable m{SimpleMovable{}};
    for (auto _ : state) {
        m.Update(0.016f);
    }
}
BENCHMARK(BM_AnyMovable_Update_Small)->Repetitions(3)->ReportAggregatesOnly(false);

static void BM_AnyMovable_Update_Large(benchmark::State& state) {
    AnyMovable m{LargeMovable{}};
    for (auto _ : state) {
        m.Update(0.016f);
    }
}
BENCHMARK(BM_AnyMovable_Update_Large)->Repetitions(3)->ReportAggregatesOnly(false);

static void BM_AnyMovable_Update_Bulk(benchmark::State& state) {
    std::vector<AnyMovable> movables;
    int count = state.range(0);
    for (int i = 0; i < count; ++i) {
        if (i % 2 == 0) movables.emplace_back(SimpleMovable{});
        else movables.emplace_back(LargeMovable{});
    }
    
    for (auto _ : state) {
        for (auto& m : movables) {
            m.Update(0.016f);
        }
    }
    state.SetComplexityN(count);
}
BENCHMARK(BM_AnyMovable_Update_Bulk)
->Range(10, 1000)
->Complexity(benchmark::oN)
->Repetitions(3)
->ReportAggregatesOnly(false);