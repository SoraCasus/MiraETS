/**
 * @file SystemScheduler.hpp
 * @brief Optimized scheduler for executing systems with batch-based dependency graph execution.
 */

#pragma once

#include <Mira/ETS/ThreadPool.hpp>
#include <algorithm>
#include <execution>
#include <vector>
#include <chrono>
#include <ranges>
#include <future>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <set>
#include <latch>

namespace Mira::ETS {
    /**
     * @brief Simple 2D vector structure.
     */
    struct Vec2 {
        float X, Y;
    };

    /**
     * @brief Run physics update in parallel using C++ execution policies.
     */
    void
    RunPhysicsParallel( std::vector < Vec2 >& positions, const std::vector < Vec2 >& velocities, float dt );

    /**
     * @brief Run physics update sequentially.
     */
    void
    RunPhysicsSequential( std::vector < Vec2 >& positions, const std::vector < Vec2 >& velocities, float dt );

    /**
     * @brief Manages and executes systems in the ECS world.
     */
    class SystemScheduler {
    public:
        SystemScheduler();

        /**
         * @brief Add a system to be executed.
         * @param system Function or lambda representing the system
         */
        void
        AddSystem( std::function < void() > system );

        /**
         * @brief Add a named system with dependencies.
         * @param name Unique name for the system
         * @param system Function or lambda representing the system
         * @param dependencies List of system names that must complete before this system starts
         */
        void
        AddSystem( std::string name, std::function < void() > system, std::vector < std::string > dependencies = {} );

        /**
         * @brief Run all added systems sequentially.
         */
        void
        RunSequential();

        /**
         * @brief Run all added systems in parallel.
         */
        void
        RunParallel();

        /**
         * @brief Run systems based on their dependency graph.
         * Independent systems will run in parallel.
         */
        void
        RunGraph();

        /**
         * @brief Rebuild the dependency graph batches.
         * This is called automatically by RunGraph if the graph has changed.
         */
        void
        RebuildGraph();

        /**
         * @brief Task-based frame execution using Thread Pool
         * @tparam Systems Variadic pack of system types
         * @param systems Systems to execute in parallel
         */
        template< typename ... Systems >
        void
        Frame( Systems&& ... systems ) {
            if constexpr ( sizeof...( Systems ) == 0 ) return;
            std::latch latch( sizeof...( Systems ) );
            ( m_Pool->Submit( [ & ] {
                systems();
                latch.count_down();
            } ), ... );
            latch.wait();
        }

    private:
        struct SystemNode {
            std::string Name;
            std::function < void() > Func;
            std::vector < std::string > Dependencies;
            std::vector < std::string > Dependents;
        };

        std::vector < std::function < void() > > m_Systems;
        std::unordered_map < std::string, SystemNode > m_Graph;
        std::vector < std::vector < std::string > > m_BatchedGraph;
        bool m_GraphDirty = true;
        std::unique_ptr < ThreadPool > m_Pool;
    };
} // namespace Mira::ETS