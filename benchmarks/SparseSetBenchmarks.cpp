#include <benchmark/benchmark.h>
#include "Mira/ETS/SparseSet.hpp"
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>

using namespace Mira::ETS;

static void BM_SparseSet_Insert(benchmark::State& state) {
    SparseSet<int> set;
    uint32_t i = 0;
    for (auto _ : state) {
        set.Insert(static_cast<EntityID>(i++), 42);
    }
}
BENCHMARK(BM_SparseSet_Insert)->Repetitions(3)->ReportAggregatesOnly(false);

static void
BM_SparseSet_Contains( benchmark::State& state ) {
    SparseSet < int > set;
    int count = state.range( 0 );
    for ( int i = 0; i < count; ++i ) {
        set.Insert( static_cast < EntityID >( i ), i );
    }

    uint32_t i = 0;
    for ( auto _ : state ) {
        benchmark::DoNotOptimize( set.Contains( static_cast < EntityID >( i % count ) ) );
        i++;
    }
    state.SetComplexityN( count );
}

BENCHMARK( BM_SparseSet_Contains )
->Range( 10, 10000 )
->Complexity( benchmark::o1 )
->Repetitions( 3 )
->ReportAggregatesOnly( false );

static void
BM_SparseSet_Get( benchmark::State& state ) {
    SparseSet < int > set;
    int count = state.range( 0 );
    for ( int i = 0; i < count; ++i ) {
        set.Insert( static_cast < EntityID >( i ), i );
    }

    uint32_t i = 0;
    for ( auto _ : state ) {
        benchmark::DoNotOptimize( set.Get( static_cast < EntityID >( i % count ) ) );
        i++;
    }
    state.SetComplexityN( count );
}

BENCHMARK( BM_SparseSet_Get )
->Range( 10, 10000 )
->Complexity( benchmark::o1 )
->Repetitions( 3 )
->ReportAggregatesOnly( false );

static void
BM_SparseSet_Remove( benchmark::State& state ) {
    SparseSet < int > set;
    int count = state.range( 0 );

    for ( auto _ : state ) {
        state.PauseTiming();
        for ( int i = 0; i < count; ++i ) {
            set.Insert( static_cast < EntityID >( i ), i );
        }
        state.ResumeTiming();

        for ( int i = 0; i < count; ++i ) {
            set.Remove( static_cast < EntityID >( i ) );
        }
    }
    state.SetComplexityN( count );
}

BENCHMARK( BM_SparseSet_Remove )
->Range( 10, 10000 )
->Complexity( benchmark::oN )
->Repetitions( 3 )
->ReportAggregatesOnly( false );

static void
BM_SparseSet_Iteration( benchmark::State& state ) {
    SparseSet < int > set;
    int count = state.range( 0 );
    for ( int i = 0; i < count; ++i ) {
        set.Insert( static_cast < EntityID >( i ), i );
    }

    for ( auto _ : state ) {
        for ( auto& val : set ) {
            benchmark::DoNotOptimize( val );
        }
    }
    state.SetComplexityN( count );
}

BENCHMARK( BM_SparseSet_Iteration )
->Range( 10, 10000 )
->Complexity( benchmark::oN )
->Repetitions( 3 )
->ReportAggregatesOnly( false );