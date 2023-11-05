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
option(BUILD_THREADPOOL_SHARED_LIBS "Build using shared libraries" ON)

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


# Include directories for the target
target_include_directories(${PROJECT_NAME}
    PUBLIC 
        $<BUILD_INTERFACE:${INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Check if this is the main project or if it is being included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Add an executable target for the example/main.cpp
    add_executable(${PROJECT_NAME}_example example/main.cpp)
    target_link_libraries(${PROJECT_NAME}_example PRIVATE ${PROJECT_NAME})
endif()

if(MSVC)
    # Replace -O3 with /O2 for MSVC, also set runtime library flags
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /MD")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /MDd")
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${PROJECT_NAME} PRIVATE /O2)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE /Od /Zi)
        target_link_options(${PROJECT_NAME} PRIVATE /DEBUG)
    endif()
else()
    # Original options for GCC/Clang
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${PROJECT_NAME} PRIVATE -O3)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE -g -O0)
    endif()
endif()


# Enable testing and add the test directory if this is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(test)
endif()

# Installation and Export Settings
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY ${INCLUDE_DIR}/${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Doxygen (only if it's the main project)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    find_package(Doxygen)
    if(Doxygen_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating documentation with Doxygen"
            VERBATIM)
    endif()
endif()

# Export the targets to a script
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)