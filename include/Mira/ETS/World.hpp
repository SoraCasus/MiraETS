//
// Copyright (c) 2025 Mirror Domain Studios. All rights reserved.
//

/**
 * @file World.hpp
 * @brief Main ECS World class and component management.
 */

#pragma once

#include <Mira/ETS/SparseSet.hpp>
#include <Mira/ETS/Observer.hpp>
#include <Mira/ETS/ComponentMask.hpp>
#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <any>
#include <utility>
#include <functional>
#include <tuple>

namespace Mira::ETS {
    namespace Internal {
        size_t
        GetNextComponentID();
    }

    template< typename T >
    struct ComponentID {
        static size_t
        Value() {
            static size_t s_ID = Internal::GetNextComponentID();
            return s_ID;
        }
    };


    /**
     * @brief The World class manages entities and their components.
     */
    class World {
    public:
        /**
         * @brief Create a new entity.
         * @return EntityID The unique ID of the new entity.
         */
        EntityID
        CreateEntity();

        /**
         * @brief Create multiple entities at once.
         * @param count The number of entities to create.
         * @return std::vector<EntityID> A vector containing the IDs of the newly created entities.
         */
        std::vector < EntityID >
        CreateEntitiesBulk( size_t count );

        /**
         * @brief Create or recreate an entity with a specific ID.
         * @param id The ID to use for the entity.
         * @return EntityID The ID of the entity.
         * @note If the entity already exists and is alive, it returns the existing ID.
         * If the entity index is already in use but with a different generation, it will be updated.
         */
        EntityID
        CreateEntity( EntityID id );

        /**
         * @brief Destroy an entity and all its components.
         * @param id The ID of the entity to destroy.
         * @note Optimized to only iterate over stores that contain the entity's components.
         */
        void
        DestroyEntity( EntityID id );

        /**
         * @brief Register a callback for a component event.
         * @tparam T Component type
         * @tparam Func Callback function type
         * @param event The event to listen for
         * @param callback The callback function
         */
        template< typename T, typename Func >
        void
        OnEvent( ComponentEvent event, Func&& callback ) {
            EnsureSignalStorage < T >();
            auto& signals = GetSignals < T >();
            switch ( event ) {
                case ComponentEvent::Added:
                    signals.onAdded.push_back( std::forward < Func >( callback ) );
                    break;
                case ComponentEvent::Removed:
                    signals.onRemoved.push_back( std::forward < Func >( callback ) );
                    break;
                case ComponentEvent::Modified:
                    signals.onModified.push_back( std::forward < Func >( callback ) );
                    break;
            }
        }

        /**
         * @brief Update a component and trigger the Modified event.
         * @tparam T Component type
         * @tparam Func Function type that takes a reference to the component
         * @param id Entity ID
         * @param func Function to update the component
         */
        template< typename T, typename Func >
        void
        PatchComponent( EntityID id, Func&& func ) {
            if ( HasComponent < T >( id ) ) {
                auto& component = GetComponent < T >( id );
                func( component );
                TriggerEvent < T >( id, ComponentEvent::Modified );
            }
        }

        /**
         * @brief Add a component to an entity.
         * @tparam T Component type
         * @param id Entity ID
         * @param component Component data
         */
        template< typename T >
        void
        AddComponent( EntityID id, T component ) {
            uint32_t index = Internal::GetIndex( id );
            if ( index >= m_EntitySignatures.size() ) {
                m_EntitySignatures.resize( index + 1 );
            }

            auto& store = GetStore < T >();
            store.Insert( id, std::move( component ) );

            size_t compId = ComponentID < std::remove_cvref_t < T > >::Value();
            m_EntitySignatures[ index ].Set( compId );

            if ( compId >= m_StoresByID.size() ) {
                m_StoresByID.resize( compId + 1, nullptr );
            }
            m_StoresByID[ compId ] = &store;

            EnsureSignalStorage < T >();
            TriggerEvent < T >( id, ComponentEvent::Added );
        }

        /**
         * @brief Remove a component from an entity.
         * @tparam T Component type
         * @param id Entity ID
         */
        template< typename T >
        void
        RemoveComponent( EntityID id ) {
            uint32_t index = Internal::GetIndex( id );
            if ( index < m_EntitySignatures.size() && HasComponent < T >( id ) ) {
                TriggerEvent < T >( id, ComponentEvent::Removed );
                GetStore < T >().Remove( id );
                m_EntitySignatures[ index ].Reset( ComponentID < std::remove_cvref_t < T > >::Value() );
            }
        }

        /**
         * @brief Get a reference to a component of an entity.
         * @tparam T Component type
         * @param id Entity ID
         * @return T& Reference to the component
         */
        template< typename T >
        T&
        GetComponent( EntityID id ) {
            return GetStore < T >().Get( id );
        }

        /**
         * @brief Check if an entity has a component of type T.
         * @tparam T Component type
         * @param id Entity ID
         * @return true if the entity has the component
         */
        template< typename T >
        bool
        HasComponent( EntityID id ) const {
            uint32_t index = Internal::GetIndex( id );
            if ( index >= m_EntitySignatures.size() ) return false;
            return m_EntitySignatures[ index ].Test( ComponentID < std::remove_cvref_t < T > >::Value() );
        }

        /**
         * @brief Check if an entity has a component by ID.
         * @param id Entity ID
         * @param componentID Component type ID
         * @return true if the entity has the component
         */
        bool
        HasComponent( EntityID id, size_t componentID ) const {
            uint32_t index = Internal::GetIndex( id );
            if ( index >= m_EntitySignatures.size() ) return false;
            return m_EntitySignatures[ index ].Test( componentID );
        }

        /**
         * @brief Generic View for querying entities with specific components.
         * @tparam Components Component types to filter by.
         */
        template< typename ... Components >
        class View {
        public:
            /**
             * @brief Construct a new View.
             * @param world World to query.
             */
            View( World& world ) :
                m_World( world ) {
                (m_ComponentMask.Set( ComponentID < std::remove_cvref_t < Components > >::Value() ), ...);
            }

            /**
             * @brief Iterate over all entities that have all requested components.
             * @tparam Func Callback function type.
             * @param func Callback function taking (EntityID, Components&...).
             */
            template< typename Func >
            void
            Each( Func&& func ) {
                if constexpr ( sizeof...( Components ) == 0 ) return;

                // Pre-fetch all stores into a tuple of references
                auto stores = std::tie( m_World.GetStore < std::remove_cvref_t < Components > >() ... );

                // Find the smallest sparse set to iterate over
                size_t minSize = std::numeric_limits < size_t >::max();
                void* smallestStorePtr = nullptr;

                auto findSmallest = [&]<size_t... Is>( std::index_sequence < Is ... > ) {
                    (( std::get < Is >( stores ).Size() < minSize
                           ? ( minSize = std::get < Is >( stores ).Size(), smallestStorePtr = &std::get <
                                   Is >( stores ) )
                           : 0 ), ...);
                };
                findSmallest( std::make_index_sequence < sizeof...( Components ) >{} );

                if ( !smallestStorePtr ) return;

                // Helper to call func with components from all stores, using direct access for the driving store
                auto callEach = [&]<size_t DrivingIdx, size_t... Is>( std::index_sequence < Is ... > ) {
                    auto& drivingStore = std::get < DrivingIdx >( stores );
                    if ( ( void* ) &drivingStore == smallestStorePtr ) {
                        const auto& entities = drivingStore.GetEntities();
                        using DrivingT = std::remove_cvref_t < std::tuple_element_t < DrivingIdx, std::tuple <
                            Components ... > > >;

                        if constexpr ( !std::is_empty_v < DrivingT > ) {
                            auto& drivingData = drivingStore.GetData();
                            for ( size_t i = 0; i < entities.size(); ++i ) {
                                EntityID entity = entities[ i ];
                                uint32_t index = Internal::GetIndex( entity );
                                if ( m_World.m_EntitySignatures[ index ].Contains( m_ComponentMask ) ) {
                                    func( ( [&]<size_t I>() -> decltype(auto) {
                                        if constexpr ( I == DrivingIdx ) return drivingData[ i ];
                                        else return std::get < I >( stores ).Get( entity );
                                    }.template operator() < Is >() ) ... );
                                }
                            }
                        } else {
                            for ( EntityID entity : entities ) {
                                uint32_t index = Internal::GetIndex( entity );
                                if ( m_World.m_EntitySignatures[ index ].Contains( m_ComponentMask ) ) {
                                    func( std::get < Is >( stores ).Get( entity ) ... );
                                }
                            }
                        }
                        return true;
                    }
                    return false;
                };

                // Try each store as the driving store (the one that matched smallestStorePtr will execute)
                auto tryIterate = [&]<size_t... Is>( std::index_sequence < Is ... > seq ) {
                    (callEach.template operator() < Is >( seq ) || ...);
                };

                tryIterate( std::make_index_sequence < sizeof...( Components ) >{} );
            }

        private:
            World& m_World;
            ComponentMask m_ComponentMask;
        };

        /**
         * @brief Create a view for the specified component types.
         * @tparam Components Component types to include in the view.
         * @return View<Components...> The created view.
         */
        template< typename ... Components >
        View < Components ... >
        GetView() {
            return View < Components ... >( *this );
        }

        /**
         * @brief Check if an entity ID is still valid and alive.
         * @param id Entity ID.
         * @return true if the entity is alive.
         */
        bool
        IsAlive( EntityID id ) const noexcept;

        /**
         * @brief Get the total number of entities ever created (including destroyed ones).
         * @return size_t Entity count.
         */
        size_t
        GetEntityCount() const noexcept {
            return m_EntitySignatures.size();
        }

        /**
         * @brief Get an EntityID at a specific index.
         * @param index Index.
         * @return EntityID The entity ID at that index.
         */
        EntityID
        GetEntityAt( uint32_t index ) const noexcept {
            return ( static_cast < EntityID >( m_EntityGenerations[ index ] ) << 32 ) | index;
        }

        /**
         * @brief Get the component mask of an entity.
         * @param id Entity ID.
         * @return const ComponentMask& The component mask.
         */
        const ComponentMask&
        GetEntityMask( EntityID id ) const noexcept {
            return m_EntitySignatures[ static_cast < uint32_t >( id ) ];
        }

        // Helper for system updates with automatic type deduction from lambda
        template< typename Func >
        void
        SystemUpdate( Func&& func ) {
            SystemUpdateHelper( std::forward < Func >( func ), &std::remove_reference_t < Func >::operator() );
        }

        // Overload for explicit types
        template< typename Component, typename ... OtherComponents, typename Func >
        void
        SystemUpdate( Func&& func ) {
            GetView < Component, OtherComponents ... >().Each( std::forward < Func >( func ) );
        }

    private:
        struct ISignalStorage {
            virtual ~ISignalStorage() = default;
        };

        template< typename T >
        struct SignalStorage : ISignalStorage {
            std::vector < ComponentCallback < T > > onAdded;
            std::vector < ComponentCallback < T > > onRemoved;
            std::vector < ComponentCallback < T > > onModified;
        };

        template< typename T >
        void
        EnsureSignalStorage() {
            size_t id = ComponentID < T >::Value();
            if ( id >= m_OnRemovedTriggers.size() ) {
                m_OnRemovedTriggers.resize( id + 1 );
            }
            if ( !m_OnRemovedTriggers[ id ] ) {
                m_OnRemovedTriggers[ id ] = [this]( EntityID eid ) {
                    this->TriggerEvent < T >( eid, ComponentEvent::Removed );
                };
            }
        }

        template< typename T >
        SignalStorage < T >&
        GetSignals() {
            auto typeIndex = std::type_index( typeid( T ) );
            auto it = m_ComponentSignals.find( typeIndex );
            if ( it == m_ComponentSignals.end() ) {
                auto newSignals = std::make_shared < SignalStorage < T > >();
                m_ComponentSignals[ typeIndex ] = newSignals;
                return *newSignals;
            }
            return static_cast < SignalStorage < T >& >( *it->second );
        }

        template< typename T >
        void
        TriggerEvent( EntityID id, ComponentEvent event ) {
            auto it = m_ComponentSignals.find( std::type_index( typeid( T ) ) );
            if ( it != m_ComponentSignals.end() ) {
                auto signals = std::static_pointer_cast < SignalStorage < T > >( it->second );
                auto& component = GetComponent < T >( id );
                switch ( event ) {
                    case ComponentEvent::Added:
                        for ( auto& cb : signals->onAdded ) cb( id, component );
                        break;
                    case ComponentEvent::Removed:
                        for ( auto& cb : signals->onRemoved ) cb( id, component );
                        break;
                    case ComponentEvent::Modified:
                        for ( auto& cb : signals->onModified ) cb( id, component );
                        break;
                }
            }
        }

        template< typename Func, typename R, typename Class, typename ... Args >
        void
        SystemUpdateHelper( Func&& func, R ( Class::*  )( Args ... ) const ) {
            GetView < std::remove_cvref_t < Args > ... >().Each( std::forward < Func >( func ) );
        }

        template< typename Func, typename R, typename Class, typename ... Args >
        void
        SystemUpdateHelper( Func&& func, R ( Class::*  )( Args ... ) ) {
            GetView < std::remove_cvref_t < Args > ... >().Each( std::forward < Func >( func ) );
        }

        template< typename T >
        SparseSet < T >&
        GetStore() {
            auto typeIndex = std::type_index( typeid( T ) );
            auto it = m_ComponentStores.find( typeIndex );
            if ( it == m_ComponentStores.end() ) {
                auto newStore = std::make_shared < SparseSet < T > >();
                m_ComponentStores[ typeIndex ] = newStore;
                return *newStore;
            }
            return static_cast < SparseSet < T >& >( *it->second );
        }

        std::vector < ISparseSet* > m_StoresByID;
        std::unordered_map < std::type_index, std::shared_ptr < ISparseSet > > m_ComponentStores;
        std::unordered_map < std::type_index, std::shared_ptr < ISignalStorage > > m_ComponentSignals;
        std::vector < std::function < void( EntityID ) > > m_OnRemovedTriggers;
        std::vector < ComponentMask > m_EntitySignatures;
        std::vector < uint32_t > m_EntityGenerations;
        std::vector < EntityID > m_FreeEntities;
    };
} // namespace Mira::ETS
