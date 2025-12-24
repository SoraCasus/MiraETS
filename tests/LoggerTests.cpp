#include <gtest/gtest.h>
#include <Mira/ETS/Logger.hpp>
#include <vector>
#include <string>

using namespace Mira::ETS;

TEST( LoggerTest, DefaultLogging ) {
    // This just ensures it doesn't crash when using default logging
    Logger::Info( "Testing default info logging" );
    Logger::Warning( "Testing default warning logging" );
    Logger::Error( "Testing default error logging" );
}

TEST( LoggerTest, CustomCallback ) {
    struct LogEntry {
        LogLevel level;
        std::string message;
    };

    static std::vector < LogEntry > logs;
    logs.clear();

    Logger::SetCallback( []( LogLevel level, std::string_view message ) {
        logs.push_back( { level, std::string( message ) } );
    } );

    Logger::Info( "Info message" );
    Logger::Warning( "Warning message" );
    Logger::Error( "Error message" );

    ASSERT_EQ( logs.size(), 3 );

    EXPECT_EQ( logs[ 0 ].level, LogLevel::Info );
    EXPECT_EQ( logs[ 0 ].message, "Info message" );

    EXPECT_EQ( logs[ 1 ].level, LogLevel::Warning );
    EXPECT_EQ( logs[ 1 ].message, "Warning message" );

    EXPECT_EQ( logs[ 2 ].level, LogLevel::Error );
    EXPECT_EQ( logs[ 2 ].message, "Error message" );

    // Reset to default
    Logger::RestoreDefaultCallback();
}
