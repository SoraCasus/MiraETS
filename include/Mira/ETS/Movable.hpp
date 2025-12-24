//
// Copyright (c) 2025 Mirror Domain Studios. All rights reserved.
//

/**
 * @file Movable.hpp
 * @brief Type-erased wrapper for movable components.
 */

#pragma once

#include <memory>
#include <new>
#include <concepts>
#include <functional>
#include <utility>
#include <cstddef>

namespace Mira::ETS {
    /**
     * @brief Concept for types that can be updated with a time step.
     */
    template< typename T >
    concept MovableInterface = requires( T t, float dt ) {
        { t.UpdatePosition( dt ) } -> std::same_as < void >;
    };

    /**
     * @brief A type-erased container for any type satisfying MovableInterface.
     */
    class AnyMovable {
    public:
        static constexpr size_t k_BufferSize = 64;

    private:
        struct VTable {
            void
            ( * UpdateFn )( void*, float );

            void
            ( * DestroyFn )( void* );

            void
            ( * CloneFn )( const void*, void** );

            void
            ( * MoveFn )( void*, void** );

            size_t Size;
        };

        template< typename T >
        static constexpr VTable k_VTableInstance{
            .UpdateFn = []( void* ptr, float dt ) {
                static_cast < T* >( ptr )->UpdatePosition( dt );
            },
            .DestroyFn = []( void* ptr ) {
                static_cast < T* >( ptr )->~T();
            },
            .CloneFn = []( const void* src, void** dst ) {
                if constexpr ( sizeof( T ) <= k_BufferSize ) {
                    new( *dst ) T( *static_cast < const T* >( src ) );
                } else {
                    *dst = ::operator new( sizeof( T ) );
                    new( *dst ) T( *static_cast < const T* >( src ) );
                }
            },
            .MoveFn = []( void* src, void** dst ) {
                if constexpr ( sizeof( T ) <= k_BufferSize ) {
                    new( *dst ) T( std::move( *static_cast < T* >( src ) ) );
                } else {
                    *dst = ::operator new( sizeof( T ) );
                    new( *dst ) T( std::move( *static_cast < T* >( src ) ) );
                }
            },
            .Size = sizeof( T )
        };

    public:
        template< MovableInterface T >
        AnyMovable( T value ) :
            m_VTablePtr( &k_VTableInstance < T > ) {
            if constexpr ( sizeof( T ) <= k_BufferSize ) {
                m_Ptr = &m_Buffer;
            } else {
                m_Ptr = ::operator new( sizeof( T ) );
            }
            new( m_Ptr ) T( std::move( value ) );
        }

        ~AnyMovable();

        AnyMovable( const AnyMovable& other );

        AnyMovable&
        operator=( const AnyMovable& other );

        AnyMovable( AnyMovable&& other ) noexcept;

        AnyMovable&
        operator=( AnyMovable&& other ) noexcept;

        void
        Update( float dt );

        const void*
        GetVTable() const;

    private:
        alignas( std::max_align_t ) std::byte m_Buffer[ k_BufferSize ];
        void* m_Ptr = nullptr;
        const VTable* m_VTablePtr = nullptr;
    };
}