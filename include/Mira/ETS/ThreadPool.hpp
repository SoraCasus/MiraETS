//
// Copyright (c) 2025 Mirror Domain Studios. All rights reserved.
//

/**
 * @file ThreadPool.hpp
 * @brief High-performance thread pool with work-stealing and lock-free task queues.
 */

#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <array>
#include <memory>

namespace Mira::ETS {
    /**
     * @brief A lock-free work-stealing queue for tasks.
     */
    class WorkStealingQueue {
    public:
        using Task = std::function < void() >;
        using TaskPtr = std::shared_ptr < Task >;

        WorkStealingQueue() : m_Top( 0 ), m_Bottom( 0 ), m_Tasks{} {
        }

        bool
        Push( Task task ) {
            size_t b = m_Bottom.load( std::memory_order_relaxed );
            size_t t = m_Top.load( std::memory_order_acquire );
            if ( b - t >= 1024 ) return false;

            m_Tasks[ b % 1024 ].store( std::make_shared < Task >( std::move( task ) ), std::memory_order_release );
            m_Bottom.store( b + 1, std::memory_order_release );
            return true;
        }

        Task
        Pop() {
            size_t b = m_Bottom.load( std::memory_order_relaxed );
            if ( b == 0 ) return nullptr;
            b--;
            m_Bottom.store( b, std::memory_order_relaxed );
            std::atomic_thread_fence( std::memory_order_seq_cst );
            size_t t = m_Top.load( std::memory_order_relaxed );

            if ( t <= b ) {
                auto taskPtr = m_Tasks[ b % 1024 ].exchange( nullptr, std::memory_order_acq_rel );
                if ( t == b ) {
                    if ( !m_Top.compare_exchange_strong( t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed ) ) {
                        m_Bottom.store( b + 1, std::memory_order_relaxed );
                        return nullptr;
                    }
                    m_Bottom.store( b + 1, std::memory_order_relaxed );
                }
                if ( !taskPtr ) return nullptr;
                return std::move( *taskPtr );
            } else {
                m_Bottom.store( b + 1, std::memory_order_relaxed );
                return nullptr;
            }
        }

        Task
        Steal() {
            size_t t = m_Top.load( std::memory_order_acquire );
            std::atomic_thread_fence( std::memory_order_seq_cst );
            size_t b = m_Bottom.load( std::memory_order_acquire );

            if ( t < b ) {
                if ( !m_Top.compare_exchange_strong( t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed ) ) {
                    return nullptr;
                }
                auto taskPtr = m_Tasks[ t % 1024 ].exchange( nullptr, std::memory_order_acq_rel );
                if ( !taskPtr ) return nullptr;
                return std::move( *taskPtr );
            }
            return nullptr;
        }

        bool
        IsEmpty() const {
            size_t b = m_Bottom.load( std::memory_order_relaxed );
            size_t t = m_Top.load( std::memory_order_relaxed );
            return t >= b;
        }

    private:
        std::atomic < size_t > m_Top;
        std::atomic < size_t > m_Bottom;
        std::array < std::atomic < TaskPtr >, 1024 > m_Tasks;
    };

    /**
     * @brief A thread pool that manages a set of worker threads with work-stealing.
     */
    class ThreadPool {
    public:
        /**
         * @brief Construct a new Thread Pool.
         * @param threads Number of worker threads to create.
         */
        explicit
        ThreadPool( size_t threads ) :
            m_Stop( false ) {
            for ( size_t i = 0; i < threads; ++i ) {
                m_Queues.emplace_back( std::make_unique < WorkStealingQueue >() );
            }

            for ( size_t i = 0; i < threads; ++i ) {
                m_Workers.emplace_back( [this, i] {
                    WorkerLoop( i );
                } );
            }
        }

        /**
         * @brief Enqueue a task into the thread pool.
         * @tparam F Function type
         * @tparam Args Argument types
         * @param f Function to execute
         * @param args Arguments to pass to the function
         * @return std::future to track the task's progress and result
         */
        template< class F, class ... Args >
        auto
        Enqueue( F&& f, Args&& ... args ) -> std::future < std::invoke_result_t < F, Args ... > > {
            using return_type = std::invoke_result_t < F, Args ... >;
            auto task = std::make_shared < std::packaged_task < return_type() > >(
                std::bind( std::forward < F >( f ), std::forward < Args >( args ) ... )
            );
            std::future < return_type > res = task->get_future();
            Submit( [task]() {
                ( *task )();
            } );
            return res;
        }

        /**
         * @brief Submit a fire-and-forget task.
         * @param task Function to execute
         */
        void
        Submit( std::function < void() > task ) {
            if ( t_WorkerIndex != -1 && t_WorkerIndex < ( int ) m_Queues.size() ) {
                if ( m_Queues[ t_WorkerIndex ]->Push( std::move( task ) ) ) {
                    return;
                }
            }

            {
                std::unique_lock < std::mutex > lock( m_QueueMutex );
                if ( m_Stop ) throw std::runtime_error( "Submit on stopped ThreadPool" );
                m_Tasks.emplace( std::move( task ) );
            }
            m_Condition.notify_one();
        }

        ~ThreadPool() {
            {
                std::unique_lock < std::mutex > lock( m_QueueMutex );
                m_Stop = true;
            }
            m_Condition.notify_all();
            for ( std::thread& worker : m_Workers )
                worker.join();
        }

    private:
        void
        WorkerLoop( size_t index ) {
            t_WorkerIndex = ( int ) index;
            while ( true ) {
                std::function < void() > task;

                // 1. Try local queue
                task = m_Queues[ index ]->Pop();

                // 2. Try global queue
                if ( !task ) {
                    std::unique_lock < std::mutex > lock( m_QueueMutex, std::try_to_lock );
                    if ( lock && !m_Tasks.empty() ) {
                        task = std::move( m_Tasks.front() );
                        m_Tasks.pop();
                    }
                }

                // 3. Try stealing
                if ( !task ) {
                    for ( size_t i = 0; i < m_Queues.size(); ++i ) {
                        size_t victim = ( index + i + 1 ) % m_Queues.size();
                        task = m_Queues[ victim ]->Steal();
                        if ( task ) break;
                    }
                }

                if ( task ) {
                    task();
                } else {
                    if ( m_Stop ) return;

                    // Wait for work
                    std::unique_lock < std::mutex > lock( m_QueueMutex );
                    m_Condition.wait_for( lock, std::chrono::microseconds( 10 ), [this, index] {
                        return m_Stop || !m_Tasks.empty() || !m_Queues[ index ]->IsEmpty();
                    } );
                    if ( m_Stop && m_Tasks.empty() && m_Queues[ index ]->IsEmpty() ) return;
                }
            }
        }

        inline static thread_local int t_WorkerIndex = -1;

        std::vector < std::thread > m_Workers;
        std::vector < std::unique_ptr < WorkStealingQueue > > m_Queues;
        std::queue < std::function < void() > > m_Tasks;
        std::mutex m_QueueMutex;
        std::condition_variable m_Condition;
        bool m_Stop;
    };
}