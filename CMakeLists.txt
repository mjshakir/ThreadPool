cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# Get the name of the folder and use it as the project name
get_filename_component(PROJECT_NAME ${CMAKE_SOURCE_DIR} NAME)
project(${PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Print out the build type
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Set the runtime output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Offer a switch to the user for choosing the type of the library (static or shared)
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

# Force colored output
option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." OFF)
if(${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
       add_compile_options(-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
       add_compile_options(-fcolor-diagnostics)
    endif()
endif()

# Define the source and include directories
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)

# Define the source files
set(SOURCES
    ${SOURCE_DIR}/ThreadPool.cpp
    ${SOURCE_DIR}/PriorityQueue.cpp
    ${SOURCE_DIR}/ThreadTask.cpp
)

# Add library target
add_library(${PROJECT_NAME} ${SOURCES})

# Set compiler options based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
endif()

# Sanitizer options for Debug configuration
target_compile_options(${PROJECT_NAME} PUBLIC $<$<CONFIG:Debug>:-fsanitize=address>)
target_link_options(${PROJECT_NAME} PUBLIC $<$<CONFIG:Debug>:-fsanitize=address>)

# Include directories for the target
target_include_directories(${PROJECT_NAME}
    PUBLIC 
        $<BUILD_INTERFACE:${INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Add an executable target for the example/main.cpp
add_executable(${PROJECT_NAME}_example example/main.cpp)
target_link_libraries(${PROJECT_NAME}_example PRIVATE ${PROJECT_NAME})

# Sanitizer options for Debug configuration for example
target_compile_options(${PROJECT_NAME}_example PUBLIC $<$<CONFIG:Debug>:-fsanitize=address>)
target_link_options(${PROJECT_NAME}_example PUBLIC $<$<CONFIG:Debug>:-fsanitize=address>)

# Enable testing and add the test directory if this is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(test)
endif()

# Installation rules
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY ${INCLUDE_DIR}/${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Export the targets to a script
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)