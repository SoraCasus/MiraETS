#include <benchmark/benchmark.h>
#include "Mira/ETS/Prefab.hpp"

using namespace Mira::ETS;

struct PrefabComp {
    int value;
};

static void BM_Prefab_Instantiate(benchmark::State& state) {
    SerializationContext context;
    context.Register<PrefabComp>("PrefabComp",
        [](const PrefabComp& c, std::ostream& os) { os << "{\"value\":" << c.value << "}"; },
        [](const simdjson::dom::element& el) { return PrefabComp{ static_cast<int>(el["value"].get_int64().value()) }; }
    );
    
    PrefabManager manager(context);
    manager.LoadPrefabs(R"({ "Test": { "PrefabComp": {"value": 42} } })");
    
    World world;
    for (auto _ : state) {
        benchmark::DoNotOptimize(manager.Instantiate("Test", world));
    }
}
BENCHMARK(BM_Prefab_Instantiate)->Repetitions(3)->ReportAggregatesOnly(false);