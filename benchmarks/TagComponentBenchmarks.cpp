#include <benchmark/benchmark.h>
#include "Mira/ETS/World.hpp"

using namespace Mira::ETS;

struct Tag1 {};
struct Tag2 {};

static void BM_Tag_Add(benchmark::State& state) {
    World world;
    auto entity = world.CreateEntity();
    for (auto _ : state) {
        world.AddComponent(entity, Tag1{});
        world.RemoveComponent<Tag1>(entity);
    }
}
BENCHMARK(BM_Tag_Add)->Repetitions(3)->ReportAggregatesOnly(false);

static void BM_Tag_Has(benchmark::State& state) {
    World world;
    auto entity = world.CreateEntity();
    world.AddComponent(entity, Tag1{});
    for (auto _ : state) {
        benchmark::DoNotOptimize(world.HasComponent<Tag1>(entity));
    }
}
BENCHMARK(BM_Tag_Has)->Repetitions(3)->ReportAggregatesOnly(false);

static void
BM_Tag_View( benchmark::State& state ) {
    World world;
    int count = state.range( 0 );
    for ( int i = 0; i < count; ++i ) {
        auto e = world.CreateEntity();
        if ( i % 2 == 0 ) world.AddComponent( e, Tag1{} );
        if ( i % 3 == 0 ) world.AddComponent( e, Tag2{} );
    }

    for ( auto _ : state ) {
        int c = 0;
        world.GetView < Tag1, Tag2 >().Each( [&]( Tag1&, Tag2& ) {
            c++;
        } );
        benchmark::DoNotOptimize( c );
    }
    state.SetComplexityN( count );
}

BENCHMARK( BM_Tag_View )
->Range( 10, 10000 )
->Complexity( benchmark::oN )
->Repetitions( 3 )
->ReportAggregatesOnly( false );