/**
 * @file Traits.hpp
 * @brief Component traits and game entity definitions.
 */

#pragma once

#include "Mira/ETS/Concepts.hpp"
#include <string>
#include <format>


namespace Mira::ETS {

    /**
     * @brief Trait for objects that can provide a status string.
     */
    struct LoggableTrait {
        /**
         * @brief Get a status string for the object.
         * @tparam Self Type of the object (must satisfy HasIdentity and HasPosition)
         * @param self The object instance
         * @return Status string
         */
        template< typename Self >
            requires HasIdentity < std::remove_cvref_t < Self > > && HasPosition < std::remove_cvref_t < Self > >
        std::string
        GetStatusString( const Self& self ) {
            return std::format( "Entity[{}] Pos: ({:.2f}, {:.2f})", self.Id, self.X, self.Y );
        }
    };

} // namespace Mira::ETS