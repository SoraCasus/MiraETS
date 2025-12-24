#include <Mira/ETS/SystemScheduler.hpp>
#include <Mira/ETS/World.hpp>
#include <iostream>
#include <chrono>
#include <thread>

/**
 * @file main.cpp
 * @brief Parallel Systems example for Mira ETS.
 * 
 * This sample demonstrates how to use the SystemScheduler to execute multiple
 * systems in parallel using a thread pool and task-based dependencies.
 */

using namespace Mira::ETS;

struct PhysicsComponent {
    float X = 0, Y = 0;
};

struct AIComponent {
    int State = 0;
};

int
main() {
    std::cout << "--- Mira ETS: Parallel Systems Example ---" << std::endl;

    World world;
    SystemScheduler scheduler;

    // Create some entities
    for ( int i = 0; i < 100; ++i ) {
        auto e = world.CreateEntity();
        world.AddComponent( e, PhysicsComponent{ ( float ) i, ( float ) i } );
        world.AddComponent( e, AIComponent{ 0 } );
    }

    // Define systems as lambdas or functions
    auto InputSystem = []() {
        std::cout << "[Input] Processing input on thread " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
    };

    auto PhysicsSystem = [&]() {
        std::cout << "[Physics] Starting update on thread " << std::this_thread::get_id() << std::endl;
        world.SystemUpdate( []( PhysicsComponent& p ) {
            p.X += 0.1f;
            p.Y += 0.1f;
        } );
        // Simulate heavy work
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        std::cout << "[Physics] Update finished" << std::endl;
    };

    auto AISystem = [&]() {
        std::cout << "[AI] Starting update on thread " << std::this_thread::get_id() << std::endl;
        world.SystemUpdate( []( AIComponent& ai ) {
            ai.State = ( ai.State + 1 ) % 10;
        } );
        // Simulate heavy work
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        std::cout << "[AI] Update finished" << std::endl;
    };

    auto RenderSystem = []() {
        std::cout << "[Render] Drawing frame on thread " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for( std::chrono::milliseconds( 30 ) );
    };

    // 1. Simple Parallel Execution using Frame()
    // Frame() will wait for all systems to complete before returning.
    std::cout << "\n--- Part 1: Simple Parallel Execution ---" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    scheduler.Frame( PhysicsSystem, AISystem );

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast < std::chrono::milliseconds >( end - start ).count();
    std::cout << "Frame execution took " << duration << "ms" << std::endl;

    // 2. Task-Based Dependency Graph
    // Define dependencies between systems.
    std::cout << "\n--- Part 2: Task-Based Dependency Graph ---" << std::endl;

    scheduler.AddSystem( "Input", InputSystem );
    scheduler.AddSystem( "Physics", PhysicsSystem, { "Input" } );
    scheduler.AddSystem( "AI", AISystem, { "Input" } );
    scheduler.AddSystem( "Render", RenderSystem, { "Physics", "AI" } );

    start = std::chrono::high_resolution_clock::now();

    // RunGraph() executes systems based on their dependencies, 
    // maximizing parallel execution of independent branches.
    scheduler.RunGraph();

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast < std::chrono::milliseconds >( end - start ).count();
    std::cout << "Graph execution took " << duration << "ms" << std::endl;
    std::cout << "(Input [50ms] -> (Physics [100ms] || AI [100ms]) -> Render [30ms] = ~180ms total)" << std::endl;

    return 0;
}