#include <benchmark/benchmark.h>
#include "Mira/ETS/World.hpp"
#include <vector>

using namespace Mira::ETS;

struct Component1 { int a; };
struct Component2 { float b; };
struct Component3 { double c; };
struct Component4 { char d; };
struct Component5 { long e; };

static void BM_DestroyEntity_FewComponents(benchmark::State& state) {
    World world;
    size_t entityCount = state.range(0);
    std::vector<EntityID> entities;
    for (size_t i = 0; i < entityCount; ++i) {
        auto e = world.CreateEntity();
        world.AddComponent(e, Component1{1});
        world.AddComponent(e, Component2{2.0f});
        entities.push_back(e);
    }

    for (auto _ : state) {
        state.PauseTiming();
        world = World();
        entities.clear();
        for (size_t i = 0; i < entityCount; ++i) {
            auto e = world.CreateEntity();
            world.AddComponent(e, Component1{1});
            world.AddComponent(e, Component2{2.0f});
            entities.push_back(e);
        }
        state.ResumeTiming();

        for (auto e : entities) {
            world.DestroyEntity(e);
        }
    }
    state.SetComplexityN(entityCount);
}
BENCHMARK(BM_DestroyEntity_FewComponents)
->Range(10, 1000)
->Complexity(benchmark::oN)
->Repetitions(3)
->ReportAggregatesOnly(false);

static void BM_DestroyEntity_ManyStores(benchmark::State& state) {
    World world;
    size_t entityCount = state.range(0);
    
    // Register several component types to increase m_ComponentStores size
    {
        auto e = world.CreateEntity();
        world.AddComponent(e, 10);
        world.AddComponent(e, 10.0f);
        world.AddComponent(e, 10.0);
        world.AddComponent(e, (char)10);
        world.AddComponent(e, (long)10);
        world.AddComponent(e, (short)10);
        world.AddComponent(e, (unsigned int)10);
        world.AddComponent(e, (unsigned char)10);
        world.AddComponent(e, (uint8_t)1);
        world.AddComponent(e, (size_t)10);
        world.DestroyEntity(e);
    }

    std::vector<EntityID> entities(entityCount);

    for (auto _ : state) {
        state.PauseTiming();
        // Recreate entities for each iteration to have something to destroy
        for (size_t i = 0; i < entityCount; ++i) {
            entities[i] = world.CreateEntity();
            world.AddComponent(entities[i], 10);
        }
        state.ResumeTiming();

        for (auto e : entities) {
            world.DestroyEntity(e);
        }
    }
    state.SetComplexityN(entityCount);
}
BENCHMARK(BM_DestroyEntity_ManyStores)
->Range(10, 1000)
->Complexity(benchmark::oN)
->Repetitions(3)
->ReportAggregatesOnly(false);
