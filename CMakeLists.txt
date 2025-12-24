cmake_minimum_required( VERSION 3.28 )
project( ETS )

set( CMAKE_CXX_STANDARD 23 )

option(MIRA_ETS_BUILD_TESTS "Build Mira ETS unit tests" ON)
option(MIRA_ETS_BUILD_EXAMPLES "Build Mira ETS example projects" ON)
option(MIRA_ETS_ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)
option(MIRA_ETS_ENABLE_BENCHMARKS "Enable benchmarking" ON)

include(FetchContent)

if(MIRA_ETS_BUILD_TESTS)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    # Prevent GoogleTest from overriding our compiler/linker settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF)
    FetchContent_MakeAvailable(googletest)
endif()

if(MIRA_ETS_ENABLE_BENCHMARKS)
    FetchContent_Declare(
            googlebench
            URL https://github.com/google/benchmark/archive/refs/tags/v1.9.1.zip
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable( googlebench )
endif()

FetchContent_Declare(
        simdjson
        URL https://github.com/simdjson/simdjson/releases/download/v4.2.4/singleheader.zip
)
FetchContent_MakeAvailable( simdjson )

find_package(Threads REQUIRED)

add_library( ETS STATIC
             src/Mira/ETS/Logger.cpp
             src/Mira/ETS/World.cpp
             src/Mira/ETS/SystemScheduler.cpp
             src/Mira/ETS/Movable.cpp
             src/Mira/ETS/Serialization.cpp
             src/Mira/ETS/Prefab.cpp
             ${simdjson_SOURCE_DIR}/simdjson.cpp )

add_library( Mira::ETS ALIAS ETS )

target_link_libraries( ETS PUBLIC Threads::Threads )

target_compile_features( ETS PUBLIC cxx_std_23 )

target_include_directories( ETS PUBLIC
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                            $<INSTALL_INTERFACE:include>
                            $<BUILD_INTERFACE:${simdjson_SOURCE_DIR}> )

# --- Installation ---

include(GNUInstallDirs)
install(TARGETS ETS
        EXPORT ETS_Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT ETS_Targets
        FILE MiraETSTargets.cmake
        NAMESPACE Mira::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MiraETS
)

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/MiraETSConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/MiraETSConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MiraETS
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/MiraETSConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/MiraETSConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/MiraETSConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MiraETS
)

if(MIRA_ETS_ENABLE_COVERAGE AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    target_compile_options(ETS PRIVATE --coverage -fno-inline -fno-inline-small-functions -fno-default-inline)
    target_link_options(ETS PRIVATE --coverage)
endif()

if(MIRA_ETS_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(MIRA_ETS_ENABLE_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(MIRA_ETS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
