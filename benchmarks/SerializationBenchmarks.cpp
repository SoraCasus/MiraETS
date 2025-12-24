#include <benchmark/benchmark.h>
#include "Mira/ETS/Serialization.hpp"
#include <sstream>

using namespace Mira::ETS;

struct SerComponent {
    int x;
    float y;
};

static void BM_Serialization_Serialize(benchmark::State& state) {
    World world;
    SerializationContext context;
    context.Register<SerComponent>("Ser", 
        [](const SerComponent& c, std::ostream& os) { os << "{\"x\":" << c.x << ",\"y\":" << c.y << "}"; },
        [](const simdjson::dom::element& el) { return SerComponent{ (int)el["x"].get_int64(), (float)el["y"].get_double() }; }
    );

    int count = state.range(0);
    for (int i = 0; i < count; ++i) {
        auto e = world.CreateEntity();
        world.AddComponent(e, SerComponent{i, (float)i});
    }

    for (auto _ : state) {
        std::stringstream ss;
        context.Serialize(world, ss);
        benchmark::DoNotOptimize(ss.str());
    }
    state.SetComplexityN(count);
}
BENCHMARK(BM_Serialization_Serialize)
->Range(10, 1000)
->Complexity(benchmark::oN)
->Repetitions(3)
->ReportAggregatesOnly(false);

static void BM_Serialization_Deserialize(benchmark::State& state) {
    World world;
    SerializationContext context;
    context.Register<SerComponent>("Ser", 
        [](const SerComponent& c, std::ostream& os) { os << "{\"x\":" << c.x << ",\"y\":" << c.y << "}"; },
        [](const simdjson::dom::element& el) { 
            return SerComponent{ static_cast<int>(el["x"].get_int64().value()), static_cast<float>(el["y"].get_double().value()) }; 
        }
    );

    int count = state.range(0);
    World tempWorld;
    for (int i = 0; i < count; ++i) {
        auto e = tempWorld.CreateEntity();
        tempWorld.AddComponent(e, SerComponent{i, (float)i});
    }
    std::stringstream ss;
    context.Serialize(tempWorld, ss);
    std::string json = ss.str();

    for (auto _ : state) {
        World newWorld;
        context.Deserialize(newWorld, json);
    }
    state.SetComplexityN(count);
}
BENCHMARK(BM_Serialization_Deserialize)
->Range(10, 1000)
->Complexity(benchmark::oN)
->Repetitions(3)
->ReportAggregatesOnly(false);

static void BM_Serialization_SerializeBinary(benchmark::State& state) {
    World world;
    SerializationContext context;
    context.RegisterBinary<SerComponent>("Ser",
        [](const SerComponent& c, std::ostream& os) { os.write(reinterpret_cast<const char*>(&c), sizeof(c)); },
        [](std::istream& is) { SerComponent c; is.read(reinterpret_cast<char*>(&c), sizeof(c)); return c; }
    );

    int count = state.range(0);
    for (int i = 0; i < count; ++i) {
        auto e = world.CreateEntity();
        world.AddComponent(e, SerComponent{i, (float)i});
    }

    for (auto _ : state) {
        std::stringstream ss;
        context.SerializeBinary(world, ss);
        benchmark::DoNotOptimize(ss.str());
    }
    state.SetComplexityN(count);
}
BENCHMARK(BM_Serialization_SerializeBinary)
->Range(10, 1000)
->Complexity(benchmark::oN)
->Repetitions(3)
->ReportAggregatesOnly(false);

static void BM_Serialization_DeserializeBinary(benchmark::State& state) {
    World world;
    SerializationContext context;
    context.RegisterBinary<SerComponent>("Ser",
        [](const SerComponent& c, std::ostream& os) { os.write(reinterpret_cast<const char*>(&c), sizeof(c)); },
        [](std::istream& is) { SerComponent c; is.read(reinterpret_cast<char*>(&c), sizeof(c)); return c; }
    );

    int count = state.range(0);
    World tempWorld;
    for (int i = 0; i < count; ++i) {
        auto e = tempWorld.CreateEntity();
        tempWorld.AddComponent(e, SerComponent{i, (float)i});
    }
    std::stringstream ss;
    context.SerializeBinary(tempWorld, ss);
    std::string data = ss.str();

    for (auto _ : state) {
        World newWorld;
        std::stringstream iss(data);
        context.DeserializeBinary(newWorld, iss);
    }
    state.SetComplexityN(count);
}
BENCHMARK(BM_Serialization_DeserializeBinary)
->Range(10, 1000)
->Complexity(benchmark::oN)
->Repetitions(3)
->ReportAggregatesOnly(false);