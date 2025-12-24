#include <benchmark/benchmark.h>
#include "Mira/ETS/World.hpp"
#include <vector>

using namespace Mira::ETS;

struct Position {
    float x, y;
};

struct Velocity {
    float dx, dy;
};

static void
BM_World_CreateEntity( benchmark::State& state ) {
    World world;
    for ( auto _ : state ) {
        world.CreateEntity();
    }
}

BENCHMARK( BM_World_CreateEntity )->Repetitions( 3 )->ReportAggregatesOnly( false );

static void
BM_World_CreateEntity_Loop( benchmark::State& state ) {
    size_t count = state.range( 0 );
    for ( auto _ : state ) {
        state.PauseTiming();
        World world;
        state.ResumeTiming();
        std::vector < EntityID > entities;
        entities.reserve( count );
        for ( size_t i = 0; i < count; ++i ) {
            entities.push_back( world.CreateEntity() );
        }
        benchmark::DoNotOptimize( entities.data() );
    }
    state.SetComplexityN( count );
}

BENCHMARK( BM_World_CreateEntity_Loop )
->Range( 10, 10000 )
->Complexity( benchmark::oN )
->Repetitions( 3 )
->ReportAggregatesOnly( false );

static void
BM_World_CreateEntity_Bulk( benchmark::State& state ) {
    size_t count = state.range( 0 );
    for ( auto _ : state ) {
        state.PauseTiming();
        World world;
        state.ResumeTiming();
        auto entities = world.CreateEntitiesBulk( count );
        benchmark::DoNotOptimize( entities.data() );
    }
    state.SetComplexityN( count );
}

BENCHMARK( BM_World_CreateEntity_Bulk )
->Range( 10, 10000 )
->Complexity( benchmark::oN )
->Repetitions( 3 )
->ReportAggregatesOnly( false );

static void
BM_World_AddComponent( benchmark::State& state ) {
    World world;
    std::vector < EntityID > entities;
    for ( int i = 0; i < 10000; ++i ) {
        entities.push_back( world.CreateEntity() );
    }

    int i = 0;
    for ( auto _ : state ) {
        world.AddComponent( entities[ i % 10000 ], Position{ 1.0f, 2.0f } );
        i++;
    }
}

BENCHMARK( BM_World_AddComponent )->Repetitions( 3 )->ReportAggregatesOnly( false );

static void
BM_World_GetComponent( benchmark::State& state ) {
    World world;
    auto entity = world.CreateEntity();
    world.AddComponent( entity, Position{ 1.0f, 2.0f } );

    for ( auto _ : state ) {
        benchmark::DoNotOptimize( world.GetComponent < Position >( entity ) );
    }
}

BENCHMARK( BM_World_GetComponent )->Repetitions( 3 )->ReportAggregatesOnly( false );

static void
BM_World_Each( benchmark::State& state ) {
    World world;
    int count = state.range( 0 );
    for ( int i = 0; i < count; ++i ) {
        auto e = world.CreateEntity();
        world.AddComponent( e, Position{ ( float ) i, ( float ) i } );
        world.AddComponent( e, Velocity{ 1.0f, 1.0f } );
    }

    for ( auto _ : state ) {
        world.GetView < Position, Velocity >().Each( []( Position& pos, Velocity& vel ) {
            pos.x += vel.dx;
            pos.y += vel.dy;
        } );
    }
    state.SetComplexityN( count );
}

BENCHMARK( BM_World_Each )
->Range( 10, 10000 )
->Complexity( benchmark::oN )
->Repetitions( 3 )
->ReportAggregatesOnly( false );

