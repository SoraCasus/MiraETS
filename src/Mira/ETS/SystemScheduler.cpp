#include "Mira/ETS/SystemScheduler.hpp"
#include <queue>
#include <atomic>
#include <latch>

namespace Mira::ETS {
    void
    RunPhysicsParallel( std::vector < Vec2 >& positions, const std::vector < Vec2 >& velocities, float dt ) {
        auto indices = std::views::iota( 0u, ( uint32_t ) positions.size() );

        std::for_each( std::execution::par_unseq,
                       indices.begin(), indices.end(),
                       [&]( size_t i ) {
                           positions[ i ].X += velocities[ i ].X * dt;
                           positions[ i ].Y += velocities[ i ].Y * dt;
                       } );
    }

    void
    RunPhysicsSequential( std::vector < Vec2 >& positions, const std::vector < Vec2 >& velocities, float dt ) {
        auto indices = std::views::iota( 0u, ( uint32_t ) positions.size() );

        std::for_each( std::execution::seq,
                       indices.begin(), indices.end(),
                       [&]( size_t i ) {
                           positions[ i ].X += velocities[ i ].X * dt;
                           positions[ i ].Y += velocities[ i ].Y * dt;
                       } );
    }

    SystemScheduler::SystemScheduler() {
        m_Pool = std::make_unique < ThreadPool >( std::thread::hardware_concurrency() );
    }

    void
    SystemScheduler::AddSystem( std::function < void() > system ) {
        m_Systems.push_back( std::move( system ) );
    }

    void
    SystemScheduler::AddSystem( std::string name, std::function < void() > system,
                                std::vector < std::string > dependencies ) {
        SystemNode node;
        node.Name = name;
        node.Func = std::move( system );
        node.Dependencies = std::move( dependencies );

        m_Graph[ name ] = std::move( node );
        m_GraphDirty = true;

        // Update dependents
        for ( const auto& dep : m_Graph[ name ].Dependencies ) {
            if ( m_Graph.count( dep ) ) {
                m_Graph[ dep ].Dependents.push_back( name );
            }
        }

        // Check if any existing nodes depend on this new node
        for ( auto& [ otherName, otherNode ] : m_Graph ) {
            if ( otherName == name ) continue;
            for ( const auto& dep : otherNode.Dependencies ) {
                if ( dep == name ) {
                    m_Graph[ name ].Dependents.push_back( otherName );
                }
            }
        }
    }

    void
    SystemScheduler::RunSequential() {
        for ( auto& system : m_Systems ) {
            system();
        }
    }

    void
    SystemScheduler::RunParallel() {
        if ( m_Systems.empty() ) return;

        std::latch latch( m_Systems.size() );
        for ( auto& system : m_Systems ) {
            m_Pool->Submit( [ & ] {
                system();
                latch.count_down();
            } );
        }
        latch.wait();
    }

    void
    SystemScheduler::RunGraph() {
        if ( m_Graph.empty() ) return;

        if ( m_GraphDirty ) {
            RebuildGraph();
        }

        // 2. Execute batches
        for ( const auto& batch : m_BatchedGraph ) {
            if ( batch.size() == 1 ) {
                m_Graph[ batch[ 0 ] ].Func();
            } else {
                std::latch latch( batch.size() );
                for ( const auto& name : batch ) {
                    m_Pool->Submit( [ &latch, &func = m_Graph[ name ].Func ] {
                        func();
                        latch.count_down();
                    } );
                }
                latch.wait();
            }
        }
    }

    void
    SystemScheduler::RebuildGraph() {
        m_BatchedGraph.clear();
        std::unordered_map < std::string, int > inDegree;
        std::vector < std::string > currentBatch;

        for ( const auto& [ name, node ] : m_Graph ) {
            inDegree[ name ] = ( int ) node.Dependencies.size();
            if ( inDegree[ name ] == 0 ) {
                currentBatch.push_back( name );
            }
        }

        while ( !currentBatch.empty() ) {
            m_BatchedGraph.push_back( currentBatch );
            std::vector < std::string > nextBatch;
            for ( const auto& name : currentBatch ) {
                for ( const auto& dependent : m_Graph[ name ].Dependents ) {
                    if ( --inDegree[ dependent ] == 0 ) {
                        nextBatch.push_back( dependent );
                    }
                }
            }
            currentBatch = std::move( nextBatch );
        }

        // Check for cycles
        size_t totalNodes = 0;
        for ( const auto& batch : m_BatchedGraph ) {
            totalNodes += batch.size();
        }

        if ( totalNodes < m_Graph.size() ) {
            throw std::runtime_error( "SystemScheduler: Dependency cycle detected or missing dependencies." );
        }

        m_GraphDirty = false;
    }
} // namespace Mira::ETS