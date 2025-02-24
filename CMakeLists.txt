cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
#------------------------------------------------------------------------------------------
# Determine if ThreadPool is built as a standalone project or included by other projects
set(THREADPOOL_STANDALONE_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(THREADPOOL_STANDALONE_PROJECT ON)
endif()
#------------------------------------------------------------------------------------------
# Get the name of the folder and use it as the project name
get_filename_component(PARENT_DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" PARENT_DIR_NAME ${PARENT_DIR_NAME})
project(${PARENT_DIR_NAME} VERSION 2.0.0 DESCRIPTION "A Threadpool function library" LANGUAGES CXX)
#------------------------------------------------------------------------------------------
# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#------------------------------------------------------------------------------------------
# Print out the build type
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
#------------------------------------------------------------------------------------------
if(THREADPOOL_STANDALONE_PROJECT)
    if(MSVC)
        # MSVC specific warning flags
        add_library(common_options INTERFACE)
        target_compile_options(common_options INTERFACE /W4)  # Equivalent to -Wall in GCC/Clang
    else()
        # GCC/Clang specific warning flags
        add_library(common_options INTERFACE)
        target_compile_options(common_options INTERFACE -Wall -Wextra -Wpedantic -pedantic-errors -Werror=format-security -fasynchronous-unwind-tables)
    endif()
endif()
#------------------------------------------------------------------------------------------
# Set the runtime output directory only if this is a standalone project
if(THREADPOOL_STANDALONE_PROJECT)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    # Optionally, to set these directories only for specific build configurations (Debug, Release, etc.),
    # you can use the following approach:
    # set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
    # set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
    # set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
    # set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
    # set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
    # set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
    # message(STATUS "Library output directory: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    # Diagnostic output
    message(STATUS "ARCHIVE_OUTPUT_DIRECTORY: ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
    message(STATUS "LIBRARY_OUTPUT_DIRECTORY: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    message(STATUS "RUNTIME_OUTPUT_DIRECTORY: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()
#------------------------------------------------------------------------------------------
# Offer switches for the type of the library and optional builds
option(BUILD_THREADPOOL_SHARED_LIBS "Build using shared libraries" ON)
option(BUILD_THREADPOOL_SINGLETON "Enable building a singleton-based ThreadPoolManager." OFF)
option(BUILD_THREADPOOL_EXAMPLE "Build ThreadPool example." ${THREADPOOL_STANDALONE_PROJECT})
option(BUILD_THREADPOOL_TESTS "Build ThreadPool tests." ${THREADPOOL_STANDALONE_PROJECT})
option(BUILD_THREADPOOL_BENCHMARK "Build ThreadPool benchmarks." ${THREADPOOL_STANDALONE_PROJECT})
option(BUILD_THREADPOOL_DEFAULT_MODE "If ON => default to PRIORITY (1) ON, else STANDARD (0) OFF." OFF)
#------------------------------
set(BUILD_THREADPOOL_DEFAULT_ADOPTIVE_TICK "1000000" CACHE STRING "Default fallback adoptive tick")
#------------------------------
if(BUILD_THREADPOOL_DEFAULT_MODE)
    set(THREADPOOL_DEFAULT_MODE "1" CACHE STRING "..." FORCE)
    # Instead of overwriting, we append if there's already something
    if(NOT "${GLOBAL_THREADPOOL_REQUEST_LIST}" STREQUAL "")
        set(GLOBAL_THREADPOOL_REQUEST_LIST
            "${GLOBAL_THREADPOOL_REQUEST_LIST};${PROJECT_NAME}:1,${BUILD_THREADPOOL_DEFAULT_ADOPTIVE_TICK}"
            CACHE INTERNAL "Global requests" FORCE
        )
    else()
        set(GLOBAL_THREADPOOL_REQUEST_LIST
            "${PROJECT_NAME}:1,${BUILD_THREADPOOL_DEFAULT_ADOPTIVE_TICK}"
            CACHE INTERNAL "Global requests" FORCE
        )
    endif()
else()
    set(THREADPOOL_DEFAULT_MODE "0" CACHE STRING "..." FORCE)
    if(NOT "${GLOBAL_THREADPOOL_REQUEST_LIST}" STREQUAL "")
        set(GLOBAL_THREADPOOL_REQUEST_LIST
            "${GLOBAL_THREADPOOL_REQUEST_LIST};${PROJECT_NAME}:0,${BUILD_THREADPOOL_DEFAULT_ADOPTIVE_TICK}"
            CACHE INTERNAL "Global requests" FORCE
        )
    else()
        set(GLOBAL_THREADPOOL_REQUEST_LIST
            "${PROJECT_NAME}:0,${BUILD_THREADPOOL_DEFAULT_ADOPTIVE_TICK}"
            CACHE INTERNAL "Global requests" FORCE
        )
    endif()
endif()
#------------------------------
message(STATUS "THREADPOOL_STANDALONE_PROJECT: ${THREADPOOL_STANDALONE_PROJECT}")
message(STATUS "BUILD_THREADPOOL_SHARED_LIBS: ${BUILD_THREADPOOL_SHARED_LIBS}")
message(STATUS "BUILD_THREADPOOL_EXAMPLE: ${BUILD_THREADPOOL_EXAMPLE}")
message(STATUS "BUILD_THREADPOOL_TESTS: ${BUILD_THREADPOOL_TESTS}")
message(STATUS "BUILD_THREADPOOL_BENCHMARK: ${BUILD_THREADPOOL_BENCHMARK}")
message(STATUS "BUILD_THREADPOOL_SINGLETON: ${BUILD_THREADPOOL_SINGLETON}")
message(STATUS "BUILD_THREADPOOL_DEFAULT_MODE: ${BUILD_THREADPOOL_DEFAULT_MODE}")
message(STATUS "BUILD_THREADPOOL_DEFAULT_ADOPTIVE_TICK: ${BUILD_THREADPOOL_DEFAULT_ADOPTIVE_TICK}")
#------------------------------
# Force colored output
option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." OFF)
if(${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options(-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options(-fcolor-diagnostics)
    endif()
endif()
#------------------------------------------------------------------------------------------
# Include ThreadPoolUnifyRequests.cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/ThreadPoolUnifyRequests.cmake)
#------------------------------
# Actually unify requests now
_threadpool_unify_requests()
#------------------------------------------------------------------------------------------
# Define the source and include directories
set(THREADPOOL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(THREADPOOL_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
#------------------------------------------------------------------------------------------
# Define the source files
set(THREADPOOL_SOURCES
    ${THREADPOOL_SOURCE_DIR}/ThreadPool.cpp
    ${THREADPOOL_SOURCE_DIR}/ThreadMode.cpp
    ${THREADPOOL_SOURCE_DIR}/PriorityQueue.cpp
    ${THREADPOOL_SOURCE_DIR}/ThreadTask.cpp
)
#------------------------------
# Conditionally add ThreadPoolManager.cpp if the singleton is ON
if(BUILD_THREADPOOL_SINGLETON)
    list(APPEND THREADPOOL_SOURCES
        src/ThreadPoolManager.cpp
    )
endif()
#------------------------------------------------------------------------------------------
# Define the ThreadPool library target
set(THREADPOOL_LIBRARY_NAME "${THREADPOOL_PROJECT_NAME}_lib")
# Check if the library should be built as a shared library
if(BUILD_THREADPOOL_SHARED_LIBS)
    add_library(${PROJECT_NAME} SHARED ${THREADPOOL_SOURCES})
    # If needed, set properties specific to shared libraries here
else()
    add_library(${PROJECT_NAME} STATIC ${THREADPOOL_SOURCES})
    # If needed, set properties specific to static libraries here
endif()
#------------------------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PUBLIC 
    $<BUILD_INTERFACE:${THREADPOOL_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)
#------------------------------------------------------------------------------------------
# Pass final config as compile definitions
target_compile_definitions(${PROJECT_NAME}
    PUBLIC
    THREADPOOL_ENABLE_SINGLETON=$<BOOL:${BUILD_THREADPOOL_SINGLETON}>
    THREADPOOL_MODE=${THREADPOOL_MODE}
    THREADPOOL_ADOPTIVE_TICK=${THREADPOOL_ADOPTIVE_TICK}
)
#------------------------------------------------------------------------------------------
# Alias for ThreadPool library
add_library(ThreadPool::threadpool ALIAS ${PROJECT_NAME})
#------------------------------------------------------------------------------------------
# Set THREADPOOL_LIBRARIES variable
if(THREADPOOL_STANDALONE_PROJECT)
    set(THREADPOOL_LIBRARIES ThreadPool::threadpool)
else()
    set(THREADPOOL_LIBRARIES ThreadPool::threadpool PARENT_SCOPE)
endif()
message(STATUS "THREADPOOL_LIBRARIES: ${THREADPOOL_LIBRARIES}")
#------------------------------------------------------------------------------------------
# Building examples
if(BUILD_THREADPOOL_EXAMPLE)
    set(THREADPOOL_EXAMPLE_NAME "${PROJECT_NAME}_example")
    add_executable(${THREADPOOL_EXAMPLE_NAME} example/main.cpp)
    target_link_libraries(${THREADPOOL_EXAMPLE_NAME} PRIVATE ${PROJECT_NAME})
endif()
#------------------------------------------------------------------------------------------
# Compiler-specific flags
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /MD")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /MDd")
    target_compile_options(${PROJECT_NAME} PRIVATE 
        "$<$<CONFIG:Release>:/O2>" 
        "$<$<CONFIG:Debug>:/Od>" 
        "$<$<CONFIG:Debug>:/Zi>"
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_options(${PROJECT_NAME} PRIVATE /DEBUG)
    endif()

    set_target_properties(${PROJECT_NAME} PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS TRUE
    )
else()
    # For non-MSVC compilers
    target_compile_options(${PROJECT_NAME} PRIVATE 
        "$<$<CONFIG:Release>:-O3>"
    )
    # Conditionally include AddressSanitizer flags only if standalone and in Debug mode
    if(THREADPOOL_STANDALONE_PROJECT AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang|GNU")
        set(SANITIZE_FLAGS "$<$<CONFIG:Debug>:-fsanitize=address>")
        target_compile_options(${PROJECT_NAME} PRIVATE 
            "$<$<CONFIG:Debug>:-g>"
            "$<$<CONFIG:Debug>:-O0>"
            ${SANITIZE_FLAGS}
        )
        target_link_options(${PROJECT_NAME} PRIVATE ${SANITIZE_FLAGS})
    endif()
endif()
#------------------------------------------------------------------------------------------
# Enable testing and add the test directory if this is a standalone project
if(BUILD_THREADPOOL_TESTS AND THREADPOOL_STANDALONE_PROJECT)
    enable_testing()
    add_subdirectory(test)
endif()
#------------------------------------------------------------------------------------------
# Enable benchmarks and add the benchmark directory if this is a standalone project
if(BUILD_THREADPOOL_BENCHMARK AND THREADPOOL_STANDALONE_PROJECT)
    add_subdirectory(benchmark)
endif()
#------------------------------------------------------------------------------------------
# Doxygen (only if it's a standalone project)
if(DOXYGEN_FOUND AND THREADPOOL_STANDALONE_PROJECT)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating documentation with Doxygen"
        VERBATIM)
endif()
#------------------------------------------------------------------------------------------
# Export the library targets (only if it's a standalone project)
if(THREADPOOL_STANDALONE_PROJECT)
    include(GNUInstallDirs)
    install(TARGETS ${PROJECT_NAME} EXPORT ThreadpoolTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(DIRECTORY ${THREADPOOL_INCLUDE_DIR}/${THREADPOOL_PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(EXPORT ThreadpoolTargets
        FILE ThreadpoolTargets.cmake
        NAMESPACE Threadpool::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${THREADPOOL_PROJECT_NAME}
    )
endif()
#------------------------------------------------------------------------------------------