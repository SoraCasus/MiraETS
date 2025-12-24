#include <benchmark/benchmark.h>
#include "Mira/ETS/World.hpp"

using namespace Mira::ETS;

struct ObsComponent { int value; };

static void BM_Observer_OnEvent_Trigger(benchmark::State& state) {
    World world;
    auto entity = world.CreateEntity();
    
    int callCount = 0;
    world.OnEvent<ObsComponent>(ComponentEvent::Added, [&](EntityID, ObsComponent&) {
        callCount++;
    });

    for (auto _ : state) {
        world.AddComponent(entity, ObsComponent{1});
        world.RemoveComponent<ObsComponent>(entity);
    }
    benchmark::DoNotOptimize(callCount);
}
BENCHMARK(BM_Observer_OnEvent_Trigger)->Repetitions(3)->ReportAggregatesOnly(false);

static void BM_Observer_Patch_Trigger(benchmark::State& state) {
    World world;
    auto entity = world.CreateEntity();
    world.AddComponent(entity, ObsComponent{1});
    
    int callCount = 0;
    world.OnEvent<ObsComponent>(ComponentEvent::Modified, [&](EntityID, ObsComponent&) {
        callCount++;
    });

    for (auto _ : state) {
        world.PatchComponent<ObsComponent>(entity, [](ObsComponent& comp) {
            comp.value++;
        });
    }
    benchmark::DoNotOptimize(callCount);
}
BENCHMARK(BM_Observer_Patch_Trigger)->Repetitions(3)->ReportAggregatesOnly(false);