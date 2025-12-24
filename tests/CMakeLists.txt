enable_testing()

add_executable( ETS_Tests
                WorldTests.cpp
                SparseSetTests.cpp
                MovableTests.cpp
                SystemSchedulerTests.cpp
                TraitsTests.cpp
                ObserverTests.cpp
                SerializationTests.cpp
                PrefabTests.cpp
                TagComponentTests.cpp
                LoggerTests.cpp )

if ( MIRA_ETS_ENABLE_COVERAGE AND ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" ))
    target_compile_options( ETS_Tests PRIVATE --coverage -fno-inline -fno-inline-small-functions -fno-default-inline )
    target_link_options( ETS_Tests PRIVATE --coverage )
endif ()

target_link_libraries( ETS_Tests PRIVATE
                       GTest::gtest_main
                       Threads::Threads
                       Mira::ETS )

add_test( NAME ETS_Tests COMMAND ETS_Tests )
