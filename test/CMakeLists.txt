# Try Finding Installed Google Test
find_package(GTest QUIET)

# Use Installed Version or Fetch
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE # Do a shallow clone to speed up the process
    )

    # This line ensures GoogleTest uses the same runtime library
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(googletest)
    add_library(GTest::GTest ALIAS gtest)
    add_library(GTest::Main ALIAS gtest_main)
endif()

# Function to setup a test executable
function(create_test_target TARGET_NAME SOURCE_FILE)
    # Correcting the file path according to the directory structure
    add_executable(${TARGET_NAME} ${SOURCE_FILE})
    target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)
    
    # Set the output directories for test executables
    set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/testbin
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/testlib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/testlib
    )
    
    # Link the main project library and Google Test
    target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_NAME})
    if(GTest_FOUND)
        target_link_libraries(${TARGET_NAME} PRIVATE GTest::GTest GTest::Main)
    else()
        target_link_libraries(${TARGET_NAME} PRIVATE gtest gtest_main)
    endif()

    # Enable testing and add the test to CTest
    enable_testing()
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})

    # Add a label to the test so that it can be filtered with ctest
    set_tests_properties(${TARGET_NAME} PROPERTIES LABELS "unit")
endfunction()

create_test_target(PriorityQueueTest PriorityQueueTest.cpp)
create_test_target(PriorityQueueDequeTest PriorityQueueDequeTest.cpp)
create_test_target(PriorityQueueStructTest PriorityQueueStructTest.cpp)
create_test_target(PriorityQueueDegueStructTest PriorityQueueDegueStructTest.cpp)
create_test_target(ThreadTaskTest ThreadTaskTest.cpp)
create_test_target(ThreadPoolTest ThreadPoolTest.cpp)
create_test_target(ThreadPoolDisableTest ThreadPoolDisableTest.cpp)
create_test_target(ThreadPoolDequeTest ThreadPoolDequeTest.cpp)
create_test_target(ThreadPoolDisableDequeTest ThreadPoolDisableDequeTest.cpp)