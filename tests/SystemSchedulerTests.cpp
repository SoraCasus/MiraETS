//
// Copyright (c) 2025 Mirror Domain Studios. All rights reserved.
//
#include <gtest/gtest.h>
#include "Mira/ETS/SystemScheduler.hpp"
#include "Mira/ETS/SparseSet.hpp"
#include <atomic>
#include <thread>
#include <chrono>

using namespace Mira::ETS;

TEST( SystemSchedulerTests, ParallelExecution_Correctness ) {
    size_t N = 1000;
    SparseSet < Vec2 > pos;
    SparseSet < Vec2 > vel;

    for ( size_t i = 0; i < N; ++i ) {
        pos.Insert( i, { 0.0f, 0.0f } );
        vel.Insert( i, { 1.0f, 1.0f } );
    }

    // Run parallel update
    RunPhysicsParallel( pos.GetData(), vel.GetData(), 1.0f );

    // Verify all updated
    for ( size_t i = 0; i < N; ++i ) {
        ASSERT_FLOAT_EQ( pos.Get( i ).X, 1.0f );
        ASSERT_FLOAT_EQ( pos.Get( i ).Y, 1.0f );
    }
}

TEST( SystemSchedulerTests, SystemScheduler_Frame ) {
    SystemScheduler scheduler;

    std::atomic < int > ai_counter{ 0 };
    std::atomic < int > physics_counter{ 0 };

    auto ai_logic = [&]() {
        std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
        ai_counter++;
    };

    auto physics_update = [&]() {
        std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
        physics_counter++;
    };

    auto start = std::chrono::high_resolution_clock::now();
    scheduler.Frame( ai_logic, physics_update );
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration < double > diff = end - start;

    EXPECT_EQ( ai_counter.load(), 1 );
    EXPECT_EQ( physics_counter.load(), 1 );
    // Parallel execution should be faster than sequential 40ms
    EXPECT_LT( diff.count(), 0.040 );
}

TEST( SystemSchedulerTests, CoverageBoost_Extra ) {
    SystemScheduler scheduler;
    std::atomic < int > counter{ 0 };

    scheduler.AddSystem( [&]() {
        counter++;
    } );
    scheduler.AddSystem( [&]() {
        counter++;
    } );

    scheduler.RunSequential();
    EXPECT_EQ( counter.load(), 2 );

    counter = 0;
    scheduler.RunParallel();
    EXPECT_EQ( counter.load(), 2 );
}

TEST( SystemSchedulerTests, SystemSchedulerThreadPool ) {
    SystemScheduler scheduler;
    std::atomic < int > count1{ 0 };
    std::atomic < int > count2{ 0 };

    scheduler.AddSystem( [&]() {
        count1++;
    } );
    scheduler.AddSystem( [&]() {
        count2++;
    } );

    scheduler.RunParallel();

    EXPECT_EQ( count1, 1 );
    EXPECT_EQ( count2, 1 );
}

TEST( SystemSchedulerTests, RunGraph_SequentialDependencies ) {
    SystemScheduler scheduler;
    std::vector < int > executionOrder;
    std::mutex mtx;

    scheduler.AddSystem( "SystemA", [&]() {
        std::lock_guard < std::mutex > lock( mtx );
        executionOrder.push_back( 1 );
    } );

    scheduler.AddSystem( "SystemB", [&]() {
        std::lock_guard < std::mutex > lock( mtx );
        executionOrder.push_back( 2 );
    }, { "SystemA" } );

    scheduler.AddSystem( "SystemC", [&]() {
        std::lock_guard < std::mutex > lock( mtx );
        executionOrder.push_back( 3 );
    }, { "SystemB" } );

    scheduler.RunGraph();

    ASSERT_EQ( executionOrder.size(), 3 );
    EXPECT_EQ( executionOrder[ 0 ], 1 );
    EXPECT_EQ( executionOrder[ 1 ], 2 );
    EXPECT_EQ( executionOrder[ 2 ], 3 );
}

TEST( SystemSchedulerTests, RunGraph_ParallelBranches ) {
    SystemScheduler scheduler;
    std::atomic < int > counter{ 0 };
    std::atomic < bool > a_done{ false };
    std::atomic < bool > b_done{ false };
    std::atomic < bool > c_done{ false };

    scheduler.AddSystem( "A", [&]() {
        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
        a_done = true;
        counter++;
    } );

    scheduler.AddSystem( "B", [&]() {
        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
        b_done = true;
        counter++;
    } );

    scheduler.AddSystem( "C", [&]() {
        EXPECT_TRUE( a_done );
        EXPECT_TRUE( b_done );
        c_done = true;
        counter++;
    }, { "A", "B" } );

    auto start = std::chrono::high_resolution_clock::now();
    scheduler.RunGraph();
    auto end = std::chrono::high_resolution_clock::now();

    EXPECT_EQ( counter, 3 );
    EXPECT_TRUE( c_done );

    std::chrono::duration < double > diff = end - start;
    // A and B should run in parallel, so total time should be ~50ms + overhead, not 100ms.
    EXPECT_LT( diff.count(), 0.090 );
}

TEST( SystemSchedulerTests, RunGraph_CycleDetection ) {
    SystemScheduler scheduler;
    scheduler.AddSystem( "A", []() {}, { "B" } );
    scheduler.AddSystem( "B", []() {}, { "A" } );

    EXPECT_THROW( scheduler.RunGraph(), std::runtime_error );
}

TEST( SystemSchedulerTests, RunGraph_Empty ) {
    SystemScheduler scheduler;
    EXPECT_NO_THROW( scheduler.RunGraph() );
}

TEST( SystemSchedulerTests, RunPhysicsSequential_Coverage ) {
    std::vector < Vec2 > positions = { { 0, 0 }, { 1, 1 } };
    std::vector < Vec2 > velocities = { { 1, 1 }, { 1, 1 } };
    RunPhysicsSequential( positions, velocities, 1.0f );
    EXPECT_FLOAT_EQ( positions[ 0 ].X, 1.0f );
    EXPECT_FLOAT_EQ( positions[ 1 ].X, 2.0f );
}