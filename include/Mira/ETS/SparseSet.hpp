/**
 * @file SparseSet.hpp
 * @brief Efficient sparse set implementation for entity and component storage.
 * 
 * Optimized to handle Tag components (empty structs) without data allocation.
 */

#pragma once

#include <vector>
#include <limits>
#include <cassert>
#include <optional>
#include <memory>
#include <cstdint>
#include <type_traits>

namespace Mira::ETS {
    /**
     * @brief Unique identifier for an entity.
     * 
     * Entities are 64-bit integers where the lower 32 bits are the index 
     * and the upper 32 bits are the generation.
     */
    using EntityID = uint64_t;

    /**
     * @brief Value representing an invalid or null index.
     */
    constexpr size_t k_NullIndex = std::numeric_limits < size_t >::max();

    namespace Internal {
        constexpr uint32_t
        GetIndex( EntityID entity ) {
            return static_cast < uint32_t >( entity & 0xFFFFFFFF );
        }

        constexpr uint32_t
        GetGeneration( EntityID entity ) {
            return static_cast < uint32_t >( entity >> 32 );
        }

        constexpr size_t k_PageSize = 4096;

        constexpr size_t
        GetPageID( uint32_t index ) {
            return index / k_PageSize;
        }

        constexpr size_t
        GetPageOffset( uint32_t index ) {
            return index % k_PageSize;
        }
    }

    /**
     * @brief Base interface for sparse sets, allowing polymorphic removal.
     */
    class ISparseSet {
    public:
        virtual ~ISparseSet() = default;

        /**
         * @brief Remove an entity from the set.
         * @param entity Entity to remove
         */
        virtual void
        Remove( EntityID entity ) = 0;

        /**
         * @brief Check if the set contains a component for an entity.
         * @param entity Entity ID
         * @return true if the entity has a component in this set
         */
        virtual bool
        Contains( EntityID entity ) const noexcept = 0;

        /**
         * @brief Get the number of entities in this set.
         * @return size_t
         */
        virtual size_t
        Size() const noexcept = 0;

        /**
         * @brief Get the list of entities in this set.
         * @return const std::vector<EntityID>&
         */
        virtual const std::vector < EntityID >&
        GetEntities() const noexcept = 0;
    };

    /**
     * @brief Sparse set implementation for storing components of type T.
     * @tparam Component The type of component to store.
     */
    template< typename Component >
    class SparseSet : public ISparseSet {
    public:
        /**
         * @brief Insert a component for an entity.
         * @param entity Entity ID
         * @param component Component data
         */
        void
        Insert( EntityID entity, Component component );

        /**
         * @brief Remove a component from an entity.
         * @param entity Entity ID
         */
        void
        Remove( EntityID entity ) override;

        /**
         * @brief Check if the set contains a component for an entity.
         * @param entity Entity ID
         * @return true if the entity has a component in this set
         */
        bool
        Contains( EntityID entity ) const noexcept override;

        /**
         * @brief Get the component for an entity.
         * @param entity Entity ID
         * @return Reference to the component
         */
        Component&
        Get( EntityID entity );

        // STL Interator compatibility
        auto
        begin() noexcept {
            return m_DenseData.begin();
        }

        auto
        end() noexcept {
            return m_DenseData.end();
        }

        size_t
        Size() const noexcept override {
            return m_DenseData.size();
        }

        auto
        Begin() const noexcept {
            return m_DenseData.begin();
        }

        auto
        End() const noexcept {
            return m_DenseData.end();
        }

        std::vector < Component >&
        GetData() noexcept {
            return m_DenseData;
        }

        const std::vector < Component >&
        GetData() const noexcept {
            return m_DenseData;
        }

        const std::vector < EntityID >&
        GetEntities() const noexcept override {
            return m_DenseToEntity;
        }

    private:
        std::vector < Component > m_DenseData;
        std::vector < EntityID > m_DenseToEntity;
        std::vector < std::unique_ptr < size_t[ ] > > m_SparsePages;
    };

    /**
     * @brief Specialized sparse set for Tag components (empty structs).
     * 
     * Optimized to avoid allocating memory for component data, using only 
     * sparse and dense arrays for entity IDs.
     */
    template< typename Component >
        requires std::is_empty_v < Component >
    class SparseSet < Component > : public ISparseSet {
    public:
        /**
         * @brief Register a tag for an entity.
         * @param entity Entity ID
         * @param component Tag instance (ignored)
         */
        void
        Insert( EntityID entity, Component component = {} );

        /**
         * @brief Remove a tag from an entity.
         * @param entity Entity ID
         */
        void
        Remove( EntityID entity ) override;

        /**
         * @brief Check if an entity has this tag.
         * @param entity Entity ID
         * @return true if the entity has the tag
         */
        bool
        Contains( EntityID entity ) const noexcept override;

        /**
         * @brief Get the tag instance for an entity.
         * @param entity Entity ID
         * @return Reference to a static tag instance
         */
        Component&
        Get( EntityID entity );

        size_t
        Size() const noexcept override {
            return m_DenseToEntity.size();
        }

        const std::vector < EntityID >&
        GetEntities() const noexcept override {
            return m_DenseToEntity;
        }

    private:
        std::vector < EntityID > m_DenseToEntity;
        std::vector < std::unique_ptr < size_t[ ] > > m_SparsePages;
    };

    // Implementation for non-empty components
    template< typename Component >
    void
    SparseSet < Component >::Insert( EntityID entity, Component component ) {
        uint32_t index = Internal::GetIndex( entity );
        size_t pageId = Internal::GetPageID( index );
        size_t pageOffset = Internal::GetPageOffset( index );

        if ( pageId >= m_SparsePages.size() ) {
            m_SparsePages.resize( pageId + 1 );
        }

        if ( !m_SparsePages[ pageId ] ) {
            m_SparsePages[ pageId ] = std::make_unique < size_t[ ] >( Internal::k_PageSize );
            for ( size_t i = 0; i < Internal::k_PageSize; ++i ) {
                m_SparsePages[ pageId ][ i ] = k_NullIndex;
            }
        }

        size_t& denseIndex = m_SparsePages[ pageId ][ pageOffset ];
        if ( denseIndex != k_NullIndex ) {
            m_DenseData[ denseIndex ] = std::move( component );
            m_DenseToEntity[ denseIndex ] = entity;
        } else {
            denseIndex = m_DenseData.size();
            m_DenseToEntity.push_back( entity );
            m_DenseData.push_back( std::move( component ) );
        }
    }

    template< typename Component >
    void
    SparseSet < Component >::Remove( EntityID entity ) {
        uint32_t index = Internal::GetIndex( entity );
        size_t pageId = Internal::GetPageID( index );
        size_t pageOffset = Internal::GetPageOffset( index );

        if ( pageId >= m_SparsePages.size() || !m_SparsePages[ pageId ] ) {
            return;
        }

        size_t denseIndexToRemove = m_SparsePages[ pageId ][ pageOffset ];
        if ( denseIndexToRemove == k_NullIndex ) {
            return;
        }

        if ( m_DenseToEntity[ denseIndexToRemove ] != entity ) {
            return;
        }

        size_t lastDenseIndex = m_DenseData.size() - 1;

        if ( denseIndexToRemove != lastDenseIndex ) {
            EntityID lastEntity = m_DenseToEntity[ lastDenseIndex ];
            uint32_t lastEntityIndex = Internal::GetIndex( lastEntity );

            m_DenseData[ denseIndexToRemove ] = std::move( m_DenseData[ lastDenseIndex ] );
            m_DenseToEntity[ denseIndexToRemove ] = lastEntity;

            m_SparsePages[ Internal::GetPageID( lastEntityIndex ) ][ Internal::GetPageOffset( lastEntityIndex ) ] =
                    denseIndexToRemove;
        }

        m_DenseData.pop_back();
        m_DenseToEntity.pop_back();
        m_SparsePages[ pageId ][ pageOffset ] = k_NullIndex;
    }

    template< typename Component >
    bool
    SparseSet < Component >::Contains( EntityID entity ) const noexcept {
        uint32_t index = Internal::GetIndex( entity );
        size_t pageId = Internal::GetPageID( index );
        size_t pageOffset = Internal::GetPageOffset( index );

        if ( pageId >= m_SparsePages.size() || !m_SparsePages[ pageId ] ) {
            return false;
        }

        size_t denseIndex = m_SparsePages[ pageId ][ pageOffset ];
        return denseIndex != k_NullIndex && m_DenseToEntity[ denseIndex ] == entity;
    }

    template< typename Component >
    Component&
    SparseSet < Component >::Get( EntityID entity ) {
        assert( Contains( entity ) );
        uint32_t index = Internal::GetIndex( entity );
        return m_DenseData[ m_SparsePages[ Internal::GetPageID( index ) ][ Internal::GetPageOffset( index ) ] ];
    }

    // Implementation for Tag components
    template< typename Component >
        requires std::is_empty_v < Component >
    void
    SparseSet < Component >::Insert( EntityID entity, Component /*component*/ ) {
        uint32_t index = Internal::GetIndex( entity );
        size_t pageId = Internal::GetPageID( index );
        size_t pageOffset = Internal::GetPageOffset( index );

        if ( pageId >= m_SparsePages.size() ) {
            m_SparsePages.resize( pageId + 1 );
        }

        if ( !m_SparsePages[ pageId ] ) {
            m_SparsePages[ pageId ] = std::make_unique < size_t[ ] >( Internal::k_PageSize );
            for ( size_t i = 0; i < Internal::k_PageSize; ++i ) {
                m_SparsePages[ pageId ][ i ] = k_NullIndex;
            }
        }

        size_t& denseIndex = m_SparsePages[ pageId ][ pageOffset ];
        if ( denseIndex != k_NullIndex ) {
            m_DenseToEntity[ denseIndex ] = entity;
        } else {
            denseIndex = m_DenseToEntity.size();
            m_DenseToEntity.push_back( entity );
        }
    }

    template< typename Component >
        requires std::is_empty_v < Component >
    void
    SparseSet < Component >::Remove( EntityID entity ) {
        uint32_t index = Internal::GetIndex( entity );
        size_t pageId = Internal::GetPageID( index );
        size_t pageOffset = Internal::GetPageOffset( index );

        if ( pageId >= m_SparsePages.size() || !m_SparsePages[ pageId ] ) {
            return;
        }

        size_t denseIndexToRemove = m_SparsePages[ pageId ][ pageOffset ];
        if ( denseIndexToRemove == k_NullIndex ) {
            return;
        }

        if ( m_DenseToEntity[ denseIndexToRemove ] != entity ) {
            return;
        }

        size_t lastDenseIndex = m_DenseToEntity.size() - 1;

        if ( denseIndexToRemove != lastDenseIndex ) {
            EntityID lastEntity = m_DenseToEntity[ lastDenseIndex ];
            uint32_t lastEntityIndex = Internal::GetIndex( lastEntity );

            m_DenseToEntity[ denseIndexToRemove ] = lastEntity;

            m_SparsePages[ Internal::GetPageID( lastEntityIndex ) ][ Internal::GetPageOffset( lastEntityIndex ) ] =
                    denseIndexToRemove;
        }

        m_DenseToEntity.pop_back();
        m_SparsePages[ pageId ][ pageOffset ] = k_NullIndex;
    }

    template< typename Component >
        requires std::is_empty_v < Component >
    bool
    SparseSet < Component >::Contains( EntityID entity ) const noexcept {
        uint32_t index = Internal::GetIndex( entity );
        size_t pageId = Internal::GetPageID( index );
        size_t pageOffset = Internal::GetPageOffset( index );

        if ( pageId >= m_SparsePages.size() || !m_SparsePages[ pageId ] ) {
            return false;
        }

        size_t denseIndex = m_SparsePages[ pageId ][ pageOffset ];
        return denseIndex != k_NullIndex && m_DenseToEntity[ denseIndex ] == entity;
    }

    template< typename Component >
        requires std::is_empty_v < Component >
    Component&
    SparseSet < Component >::Get( [[maybe_unused]] EntityID entity ) {
        assert( Contains( entity ) );
        static Component instance{};
        return instance;
    }
} // namespace Mira::ETS