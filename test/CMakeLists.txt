# If not found, fetch it from the repository
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

macro(package_add_test TESTNAME)
    add_executable(${TESTNAME} ${ARGN})
    target_link_libraries(${TESTNAME} GTest::GTest GTest::Main ${PROJECT_NAME})
    add_test(NAME ${TESTNAME} COMMAND ${TESTNAME})
    set_target_properties(${TESTNAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/testBin)
    add_dependencies(${TESTNAME} ${PROJECT_NAME})
endmacro()

package_add_test(PriorityQueueTest PriorityQueueTest.cpp)
package_add_test(PriorityQueueDequeTest PriorityQueueDequeTest.cpp)
package_add_test(PriorityQueueStructTest PriorityQueueStructTest.cpp)
package_add_test(PriorityQueueDegueStructTest PriorityQueueDegueStructTest.cpp)
package_add_test(ThreadTaskTest ThreadTaskTest.cpp)
package_add_test(ThreadPoolTest ThreadPoolTest.cpp)
package_add_test(ThreadPoolDequeTest ThreadPoolDequeTest.cpp) 