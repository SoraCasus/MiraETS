/**
 * @file Concepts.hpp
 * @brief Defines core concepts used for component type validation in the ETS library.
 */

#pragma once

#include <concepts>
#include <format>
#include <utility>
#include <type_traits>


/**
 * @brief Macro to define a trait member with [[no_unique_address]] optimization.
 * @param TraitType The type of the trait to include.
 * @param MemberName The name of the member variable.
 */
#define MIRA_TRAIT(TraitType, MemberName) [[no_unique_address]] TraitType MemberName


namespace Mira::ETS {
    /**
     * @brief Concept for types that have X and Y float members.
     */
    template< typename T >
    concept HasPosition = requires( T t ) {
        { t.X } -> std::convertible_to < float >;
        { t.Y } -> std::convertible_to < float >;
    };

    /**
     * @brief Concept for types that have Vx and Vy float members.
     */
    template< typename T >
    concept HasVelocity = requires( T t ) {
        { t.Vx } -> std::convertible_to < float >;
        { t.Vy } -> std::convertible_to < float >;
    };

    /**
     * @brief Concept for types that have an Id member convertible to size_t.
     */
    template< typename T >
    concept HasIdentity = requires( T t ) {
        { t.Id } -> std::convertible_to < size_t >;
    };
} // namespace Mira::ETS