//
// Copyright (c) 2025 Mirror Domain Studios. All rights reserved.
//

#pragma once

#include <string_view>
#include <functional>

namespace Mira::ETS {
    /**
     * @brief Log levels for Mira ETS.
     */
    enum class LogLevel {
        Info,
        Warning,
        Error
    };

    /**
     * @brief Callback type for logging.
     * @param level The log level.
     * @param message The log message.
     */
    using LogCallback = std::function < void( LogLevel level, std::string_view message ) >;

    /**
     * @brief Static logger class for Mira ETS.
     */
    class Logger {
    public:
        /**
         * @brief Set the logging callback.
         * @param callback The new callback. If null, logging is disabled.
         */
        static void
        SetCallback( LogCallback callback );

        /**
         * @brief Restore the default logging callback (std::cout/std::cerr).
         */
        static void
        RestoreDefaultCallback();

        /**
         * @brief Log a message.
         * @param level The log level.
         * @param message The log message.
         */
        static void
        Log( LogLevel level, std::string_view message );

        /**
         * @brief Log an info message.
         * @param message The log message.
         */
        static void
        Info( std::string_view message ) {
            Log( LogLevel::Info, message );
        }

        /**
         * @brief Log a warning message.
         * @param message The log message.
         */
        static void
        Warning( std::string_view message ) {
            Log( LogLevel::Warning, message );
        }

        /**
         * @brief Log an error message.
         * @param message The log message.
         */
        static void
        Error( std::string_view message ) {
            Log( LogLevel::Error, message );
        }

    private:
        static void
        DefaultCallback( LogLevel level, std::string_view message );

        static LogCallback s_Callback;
    };
} // namespace Mira::ETS
