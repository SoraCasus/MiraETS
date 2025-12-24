#include "Mira/ETS/World.hpp"
#include <algorithm>

namespace Mira::ETS {
    namespace Internal {
        size_t
        GetNextComponentID() {
            static size_t s_NextID = 0;
            return s_NextID++;
        }
    }

    EntityID
    World::CreateEntity() {
        if ( !m_FreeEntities.empty() ) {
            EntityID id = m_FreeEntities.back();
            m_FreeEntities.pop_back();

            uint32_t index = Internal::GetIndex( id );
            uint32_t generation = m_EntityGenerations[ index ];

            return ( static_cast < EntityID >( generation ) << 32 ) | index;
        }

        uint32_t index = static_cast < uint32_t >( m_EntitySignatures.size() );
        m_EntitySignatures.emplace_back( 0 );
        m_EntityGenerations.push_back( 0 );
        return static_cast < EntityID >( index ); // Generation 0
    }

    std::vector < EntityID >
    World::CreateEntitiesBulk( size_t count ) {
        std::vector < EntityID > entities;
        entities.reserve( count );

        while ( count > 0 && !m_FreeEntities.empty() ) {
            entities.push_back( CreateEntity() );
            count--;
        }

        if ( count > 0 ) {
            uint32_t startIndex = static_cast < uint32_t >( m_EntitySignatures.size() );
            m_EntitySignatures.resize( startIndex + count, ComponentMask( 0 ) );
            m_EntityGenerations.resize( startIndex + count, 0 );

            for ( uint32_t i = 0; i < count; ++i ) {
                entities.push_back( static_cast < EntityID >( startIndex + i ) );
            }
        }

        return entities;
    }

    EntityID
    World::CreateEntity( EntityID id ) {
        uint32_t index = Internal::GetIndex( id );
        uint32_t generation = Internal::GetGeneration( id );

        if ( index >= m_EntitySignatures.size() ) {
            m_EntitySignatures.resize( index + 1 );
            m_EntityGenerations.resize( index + 1 );
        }

        if ( IsAlive( id ) ) {
            return id;
        }

        // If it was in free list, remove it
        m_FreeEntities.erase(
            std::remove_if( m_FreeEntities.begin(), m_FreeEntities.end(),
                            [index]( EntityID eid ) {
                                return Internal::GetIndex( eid ) == index;
                            } ),
            m_FreeEntities.end() );

        m_EntityGenerations[ index ] = generation;
        m_EntitySignatures[ index ].Reset();

        return id;
    }

    void
    World::DestroyEntity( EntityID id ) {
        uint32_t index = Internal::GetIndex( id );
        uint32_t generation = Internal::GetGeneration( id );

        if ( IsAlive( id ) ) {
            m_EntitySignatures[ index ].ForEachSetBit( [&]( size_t i ) {
                if ( i < m_OnRemovedTriggers.size() && m_OnRemovedTriggers[ i ] ) {
                    m_OnRemovedTriggers[ i ]( id );
                }
                if ( i < m_StoresByID.size() && m_StoresByID[ i ] ) {
                    m_StoresByID[ i ]->Remove( id );
                }
            } );
            m_EntitySignatures[ index ].Reset();
            m_EntityGenerations[ index ]++;
            m_FreeEntities.push_back( ( static_cast < EntityID >( m_EntityGenerations[ index ] ) << 32 ) | index );
        }
    }

    bool
    World::IsAlive( EntityID id ) const noexcept {
        uint32_t index = Internal::GetIndex( id );
        uint32_t generation = Internal::GetGeneration( id );
        return index < m_EntitySignatures.size() && m_EntityGenerations[ index ] == generation;
    }
} // namespace Mira::ETS
