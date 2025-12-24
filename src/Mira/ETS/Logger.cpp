#include <Mira/ETS/Logger.hpp>
#include <iostream>

namespace Mira::ETS {
    LogCallback Logger::s_Callback = Logger::DefaultCallback;

    void
    Logger::SetCallback( LogCallback callback ) {
        s_Callback = std::move( callback );
    }

    void
    Logger::RestoreDefaultCallback() {
        s_Callback = Logger::DefaultCallback;
    }

    void
    Logger::Log( LogLevel level, std::string_view message ) {
        if ( s_Callback ) {
            s_Callback( level, message );
        }
    }

    void
    Logger::DefaultCallback( LogLevel level, std::string_view message ) {
        switch ( level ) {
            case LogLevel::Info:
                std::cout << "[Mira ETS Info] " << message << std::endl;
                break;
            case LogLevel::Warning:
                std::cout << "[Mira ETS Warning] " << message << std::endl;
                break;
            case LogLevel::Error:
                std::cerr << "[Mira ETS Error] " << message << std::endl;
                break;
        }
    }
} // namespace Mira::ETS
