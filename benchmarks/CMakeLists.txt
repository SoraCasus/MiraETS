macro( mira_define_benchmark BENCHMARK_NAME )
    add_executable( ${BENCHMARK_NAME} ${ARGN} )

    target_link_libraries( ${BENCHMARK_NAME} PRIVATE
                           benchmark::benchmark_main
                           Mira::ETS
    )

    if ( MSVC )
        target_compile_options( ${BENCHMARK_NAME} PRIVATE /W4 )
    else ()
        target_compile_options( ${BENCHMARK_NAME} PRIVATE -Wall -Wextra )
    endif ()
endmacro()


set( BENCHMARK_SOURCES
     ECSBenchmarks.cpp
     MovableBenchmarks.cpp
     ObserverBenchmarks.cpp
     PrefabBenchmarks.cpp
     SerializationBenchmarks.cpp
     SparseSetBenchmarks.cpp
     SystemSchedulerBenchmarks.cpp
     TagComponentBenchmarks.cpp
     TraitsBenchmarks.cpp
     WorldBenchmarks.cpp
)

foreach ( SOURCE ${BENCHMARK_SOURCES} )
    get_filename_component( NAME ${SOURCE} NAME_WE )
    mira_define_benchmark( ${NAME} ${SOURCE} )
endforeach ()

mira_define_benchmark( ETS_Benchmarks ${BENCHMARK_SOURCES} )