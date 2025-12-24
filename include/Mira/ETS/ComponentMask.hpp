#pragma once

#include <cstdint>
#include <array>
#include <cstring>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace Mira::ETS {
    /**
     * @brief A fixed-size bitset optimized for ECS component masks.
     * Supports up to 256 component types.
     */
    class ComponentMask {
    public:
        /**
         * @brief Construct a new Component Mask.
         * @param val Initial value for the first 64 bits.
         */
        explicit
        ComponentMask( unsigned long long val = 0 ) noexcept {
            m_Words.fill( 0 );
            m_Words[ 0 ] = val;
        }

        /**
         * @brief Set a bit.
         * @param bit The bit index (0-255).
         */
        void
        Set( size_t bit ) noexcept {
            if ( bit < 256 ) {
                m_Words[ bit / 64 ] |= ( 1ULL << ( bit % 64 ) );
            }
        }

        /**
         * @brief Reset a bit.
         * @param bit The bit index (0-255).
         */
        void
        Reset( size_t bit ) noexcept {
            if ( bit < 256 ) {
                m_Words[ bit / 64 ] &= ~( 1ULL << ( bit % 64 ) );
            }
        }

        /**
         * @brief Reset all bits.
         */
        void
        Reset() noexcept {
            m_Words.fill( 0 );
        }

        /**
         * @brief Test if a bit is set.
         * @param bit The bit index (0-255).
         * @return true If the bit is set.
         */
        bool
        Test( size_t bit ) const noexcept {
            if ( bit >= 256 ) return false;
            return ( m_Words[ bit / 64 ] & ( 1ULL << ( bit % 64 ) ) ) != 0;
        }

        /**
         * @brief Check if any bit is set.
         * @return true If any bit is set.
         */
        bool
        Any() const noexcept {
            for ( uint64_t w : m_Words ) if ( w ) return true;
            return false;
        }

        /**
         * @brief Check if no bits are set.
         * @return true If no bits are set.
         */
        bool
        None() const noexcept {
            return !Any();
        }

        /**
         * @brief Get the size of the mask.
         * @return size_t The number of bits (256).
         */
        size_t
        Size() const noexcept {
            return 256;
        }

        bool
        operator==( const ComponentMask& other ) const noexcept {
            return m_Words == other.m_Words;
        }

        bool
        operator!=( const ComponentMask& other ) const noexcept {
            return m_Words != other.m_Words;
        }

        ComponentMask
        operator&( const ComponentMask& other ) const noexcept {
            ComponentMask result;
            for ( size_t i = 0; i < 4; ++i ) {
                result.m_Words[ i ] = m_Words[ i ] & other.m_Words[ i ];
            }
            return result;
        }

        ComponentMask
        operator|( const ComponentMask& other ) const noexcept {
            ComponentMask result;
            for ( size_t i = 0; i < 4; ++i ) {
                result.m_Words[ i ] = m_Words[ i ] | other.m_Words[ i ];
            }
            return result;
        }

        /**
         * @brief Check if this mask contains all bits set in another mask.
         * Optimized equivalent of (this & other) == other.
         * @param other The other mask.
         * @return true If this mask contains all bits of the other.
         */
        bool
        Contains( const ComponentMask& other ) const noexcept {
            for ( size_t i = 0; i < 4; ++i ) {
                if ( ( m_Words[ i ] & other.m_Words[ i ] ) != other.m_Words[ i ] ) return false;
            }
            return true;
        }

        /**
         * @brief Iterate over all set bits and call a function for each.
         * Uses bitmask intrinsics for high performance.
         * @tparam F Function type (size_t -> void)
         * @param f Function to call.
         */
        template< typename F >
        void
        ForEachSetBit( F&& f ) const noexcept {
            for ( size_t i = 0; i < 4; ++i ) {
                uint64_t w = m_Words[ i ];
                while ( w ) {
                    uint32_t bit;
#if defined(_MSC_VER)
                    _BitScanForward64( ( unsigned long* ) &bit, w );
#else
                    bit = __builtin_ctzll( w );
#endif
                    f( i * 64 + bit );
                    w &= ~( 1ULL << bit );
                }
            }
        }

        /**
         * @brief Get a specific 64-bit word of the mask.
         * @param index Word index (0-3).
         * @return uint64_t The word value.
         */
        uint64_t
        GetWord( size_t index ) const noexcept {
            return m_Words[ index ];
        }

    private:
        std::array < uint64_t, 4 > m_Words;
    };
} // namespace Mira::ETS
