//
// Copyright (c) 2025 Mirror Domain Studios. All rights reserved.
//

#pragma once

#include <string>

namespace Mira::ETS {
    /**
     * @brief Version information for Mira ETS.
     */
    struct Version {
        static constexpr int Major = 1;
        static constexpr int Minor = 0;
        static constexpr int Patch = 0;

        /**
         * @brief Get the version as a string.
         * @return std::string Version string (e.g., "1.0.0")
         */
        static std::string
        ToString() {
            return std::to_string( Major ) + "." + std::to_string( Minor ) + "." + std::to_string( Patch );
        }
    };
} // namespace Mira::ETS
