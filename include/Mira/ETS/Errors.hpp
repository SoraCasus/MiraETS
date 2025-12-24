#pragma once

#include <string>
#include <string_view>
#include <Mira/ETS/Logger.hpp>

namespace Mira::ETS {
    enum class ErrorCode {
        None = 0,
        InvalidJson,
        MissingField,
        TypeMismatch,
        ComponentNotRegistered,
        UnknownPrefab,
        InternalError
    };

    struct Result {
        ErrorCode code = ErrorCode::None;
        std::string message;

        bool
        Success() const {
            return code == ErrorCode::None;
        }

        operator bool() const {
            return Success();
        }

        static Result
        Ok() {
            return { ErrorCode::None, "" };
        }

        static Result
        Error( ErrorCode code, std::string message ) {
            return { code, std::move( message ) };
        }
    };

    /**
     * @brief Simple error reporter that can be customized.
     */
    class ErrorReporter {
    public:
        virtual ~ErrorReporter() = default;

        virtual void
        Report( const Result& result ) {
            if ( !result.Success() ) {
                Logger::Error( std::to_string( ( int ) result.code ) + ": " + result.message );
            }
        }
    };

    inline ErrorReporter DefaultErrorReporter;
} // namespace Mira::ETS