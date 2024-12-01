# Try Finding Installed Google Benchmark
find_package(benchmark QUIET)
#------------------------------------------------------------------------------------------
# Use Installed Version or Fetch
if(NOT benchmark_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE # Do a shallow clone to speed up the process
    )

    # This line ensures Google Benchmark uses the same runtime library
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googlebenchmark)
    add_library(benchmark::benchmark ALIAS benchmark)
endif()
#------------------------------------------------------------------------------------------
# Function to setup a benchmark executable
function(create_benchmark_target TARGET_NAME SOURCE_FILE)
    # Correcting the file path according to the directory structure
    add_executable(${TARGET_NAME} ${SOURCE_FILE})
    target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)
    
    # Set the output directories for benchmark executables
    set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarkbin
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarklib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarklib
    )
    
    # Link the main project library and Google Benchmark
    target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_NAME})
    if(benchmark_FOUND)
        target_link_libraries(${TARGET_NAME} PRIVATE benchmark::benchmark)
    else()
        target_link_libraries(${TARGET_NAME} PRIVATE benchmark)
    endif()

    # Enable testing and add the benchmark to CTest
    enable_testing()
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
endfunction()
#------------------------------------------------------------------------------------------
# Add the benchmarks without the 'benchmark/' prefix since we are already in the benchmark directory
create_benchmark_target(${PROJECT_NAME}_PriorityQueue_Benchmark PriorityQueueBenchmark.cpp)
create_benchmark_target(${PROJECT_NAME}_ThreadTask_Benchmark ThreadTaskBenchmark.cpp)
create_benchmark_target(${PROJECT_NAME}_ThreadPool_Benchmark ThreadPoolBenchmark.cpp)
#------------------------------------------------------------------------------------------