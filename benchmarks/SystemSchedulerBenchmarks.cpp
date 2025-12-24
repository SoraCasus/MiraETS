#include <benchmark/benchmark.h>
#include "Mira/ETS/SystemScheduler.hpp"
#include <cmath>

using namespace Mira::ETS;

static void Work(int amount) {
    double val = 0;
    for (int i = 0; i < amount; ++i) {
        val += std::sin(i) * std::cos(i);
    }
    benchmark::DoNotOptimize(val);
}

static void BM_Scheduler_Sequential(benchmark::State& state) {
    SystemScheduler scheduler;
    int systemCount = state.range(0);
    int workAmount = state.range(1);
    
    for (int i = 0; i < systemCount; ++i) {
        scheduler.AddSystem([workAmount]() { Work(workAmount); });
    }
    
    for (auto _ : state) {
        scheduler.RunSequential();
    }
}
BENCHMARK( BM_Scheduler_Sequential )->Args( { 10, 1000 } )->Args( { 100, 100 } )->Repetitions( 3 )->ReportAggregatesOnly( false );

static void
BM_Scheduler_Parallel( benchmark::State& state ) {
    SystemScheduler scheduler;
    int systemCount = state.range( 0 );
    int workAmount = state.range( 1 );

    for ( int i = 0; i < systemCount; ++i ) {
        scheduler.AddSystem( [workAmount]() { Work( workAmount ); } );
    }

    for ( auto _ : state ) {
        scheduler.RunParallel();
    }
}

BENCHMARK( BM_Scheduler_Parallel )->Args( { 10, 1000 } )->Args( { 100, 100 } )->Repetitions( 3 )->ReportAggregatesOnly( false );

static void
BM_Scheduler_Graph( benchmark::State& state ) {
    SystemScheduler scheduler;
    int systemCount = state.range( 0 );
    int workAmount = state.range( 1 );

    for ( int i = 0; i < systemCount; ++i ) {
        std::string name = "sys" + std::to_string( i );
        std::vector < std::string > deps;
        if ( i > 0 ) {
            deps.push_back( "sys" + std::to_string( i - 1 ) );
        }
        scheduler.AddSystem( name, [workAmount]() { Work( workAmount ); }, deps );
    }

    for ( auto _ : state ) {
        scheduler.RunGraph();
    }
}

BENCHMARK( BM_Scheduler_Graph )->Args( { 10, 1000 } )->Repetitions( 3 )->ReportAggregatesOnly( false );