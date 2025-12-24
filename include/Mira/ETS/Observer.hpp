/**
 * @file Observer.hpp
 * @brief Observer patterns and event definitions for component changes.
 */

#pragma once

#include <Mira/ETS/SparseSet.hpp>
#include <functional>

namespace Mira::ETS {
    /**
     * @brief Events triggered by component operations.
     */
    enum class ComponentEvent {
        Added,
        Removed,
        Modified
    };

    /**
     * @brief Callback type for component events.
     */
    template< typename Component >
    using ComponentCallback = std::function < void( EntityID, Component & ) >;
} // namespace Mira::ETS