# ThreadPool

| Architecture | Ubuntu | macOS | Windows |
|--------------|--------|-------|---------|
| **x86_64**   | ![Ubuntu X86_64](https://github.com/mjshakir/ThreadPool/actions/workflows/ubuntu_X86_64.yml/badge.svg) | ![macOS X86_64](https://github.com/mjshakir/ThreadPool/actions/workflows/macos_x86_64.yml/badge.svg) | ![Windows X86_64](https://github.com/mjshakir/ThreadPool/actions/workflows/windows_x86_64.yml/badge.svg) |
| **ARM**      | ![Ubuntu ARM](https://github.com/mjshakir/ThreadPool/actions/workflows/ubuntu_arm.yml/badge.svg) | ![macOS ARM](https://github.com/mjshakir/ThreadPool/actions/workflows/macos_arm.yml/badge.svg) | ![Windows ARM](https://github.com/mjshakir/ThreadPool/actions/workflows/windows_arm.yml/badge.svg) |
| **RISCV**    | ![Ubuntu RISCV](https://github.com/mjshakir/ThreadPool/actions/workflows/ubuntu_riscv.yml/badge.svg) |        |         |

ThreadPool is a high-performance C++ library designed for efficient parallel execution of tasks, utilizing a pool of managed threads. Under the hood, it leverages the power of `std::jthread` introduced in `C++20`, which automatically manages the life cycle of threads—saving developers from the intricacies of explicit thread management. **One standout feature is its optional singleton ThreadPoolManager, which unifies configuration requests across multiple submodules, ensuring a consistent thread pool configuration across your entire application.**

This library stands out by offering flexibility in task scheduling through either a standard Deque or a custom-implemented `PriorityQueue`. This feature provides refined control over task execution order, allowing priority-based task handling. Whether you are dealing with simple parallel tasks or need advanced control over task execution order, ThreadPool can cater to those requirements with ease and efficiency.

## Key Features

- Modern C++ implementation utilizing `std::jthread` for automatic thread management and reduced overhead in resource handling.
- Flexible task scheduling mechanisms including standard Deque or priority-based PriorityQueue, enabling refined control over task execution based on user-defined priorities.
- **Optional Singleton ThreadPoolManager:** Unify configuration requests from multiple modules so that a single, globally accessible thread pool instance is used throughout your application.
- Simplified integration with existing CMake projects, making it straightforward to include in your project.
- Easy management of a pool of threads.
- Task execution using either a `Deque` or a `PriorityQueue`.
- Customizable task prioritization (when using the `PriorityQueue`).
- Support for `C++20`.
- Options for both shared and static library build configurations to suit various application needs.


## Prerequisites
- CMake 3.15 or higher
- A compatible C++20 compiler

## How to Include ThreadPool in Your Project

You can easily include ThreadPool into your CMake project. Here is a step-by-step guide:

1. Add ThreadPool as a submodule to your project git repository.
    ```bash
    git submodule add https://github.com/mjshakir/ThreadPool.git extern/ThreadPool
    ```

2. Add the following lines to your CMakeLists.txt file.
    ```cmake
    add_subdirectory(extern/ThreadPool) # build shared library

    # Link your application with ThreadPool
    target_link_libraries(your_target_name PRIVATE ${THREADPOOL_LIBRARIES}) # or target_link_libraries(your_target_name PRIVATE ThreadPool::threadpool)
    ```

### Choosing Library Type (Static or Shared)

By default, ThreadPool is built as a shared library. If you prefer a static library, you can set the `BUILD_THREADPOOL_SHARED_LIBS` option to `OFF` when generating the build system files:

```cmake
set(BUILD_THREADPOOL_SHARED_LIBS OFF) # for static library
add_subdirectory(extern/ThreadPool)
```

## Build and Installation

Building the `ThreadPool` project is a straightforward process, and we provide steps for using either the traditional `make` or the faster `Ninja` build system.

### Using Make

1. Clone the repository.
    ```bash
    git clone https://github.com/mjshakir/ThreadPool.git
    ```

    ```bash
    cd ThreadPool
    ```

2. Create a build directory and generate build system files.
    ```bash
    mkdir build; cd build
    ```

    ```bash
    cmake -DFORCE_COLORED_OUTPUT=ON -DCMAKE_BUILD_TYPE=Release ..
    ```

3. Build the library.
    ```bash
    cmake --build . --config Release
    ```

4. Optionally, install the library in your system.
    ```bash
    cmake --install .
    ```

### Using Ninja

Ninja is known for its speed and is the preferred option for many developers. Follow these steps to build with Ninja:

1. Clone the repository if you haven't already.
    ```bash
    git clone https://github.com/mjshakir/ThreadPool.git
    ```

    ```bash
    cd ThreadPool
    ```
2. From the project root directory, generate the build files with Ninja. We enable colored output and set the build type to release for optimized code.
    ```bash
    cmake -DFORCE_COLORED_OUTPUT=ON -DCMAKE_BUILD_TYPE=Release -B build -G Ninja
    ```

3. Build the project. Since we're using Ninja, this step should be significantly faster compared to traditional methods.
    ```bash
    cd build
    ```
    ```bash
    ninja
    ```

Note: If you haven't installed Ninja, you can do so by following the instructions on [Ninja's GitHub page](https://github.com/ninja-build/ninja).

## Advanced Configuration and Integration

### Standalone Build Options

When building ThreadPool as a standalone project, you can customize its behavior using several CMake options:

- **BUILD_THREADPOOL_SINGLETON**  
  Enables a singleton-based `ThreadPoolManager`.  
  *Default: OFF*  
  *Usage:* Set to `ON` to have a single, globally accessible instance of the thread pool.

- **BUILD_THREADPOOL_DEFAULT_MODE**  
  Specifies the default scheduling mode if no submodule requests are made.  
  - `ON` defaults to PRIORITY (`1`).
  - `OFF` defaults to STANDARD (`0`).  
  *Default: OFF (STANDARD)*

- **BUILD_THREADPOOL_DEFAULT_ADOPTIVE_TICK**  
  Sets the default adaptive tick value.  
  A tick value of `0` means “non‑adaptive” behavior and takes precedence over any nonzero (adaptive) value.  
  *Default: 1000000*

These options control the ThreadPool behavior when built on its own

- **Example:**
```bash
cmake -DFORCE_COLORED_OUTPUT=ON -DBUILD_THREADPOOL_SINGLETON=ON -DBUILD_THREADPOOL_DEFAULT_MODE=0 -DBUILD_THREADPOOL_DEFAULT_ADOPTIVE_TICK=1500000 -DCMAKE_BUILD_TYPE=Release -B build -G Ninja
```

### Integrating ThreadPool as a Submodule

When using `ThreadPool` as a submodule (for example, when projects `A`, `B`, `C`, etc. are included in a larger project `E`), you need to ensure that all configuration requests from the parent and subprojects are combined into one global configuration. This is done via a global variable called `GLOBAL_THREADPOOL_REQUEST_LIST` that the `ThreadPool` unification code reads.

#### Important:
If you simply use a `set(... FORCE)` command each time, the new value will overwrite the previous one. Instead, you should append new requests to the global list so that all requests are taken into account.

For someone not very familiar with CMake, here is a step-by-step example you can copy and paste into your parent project's `CMakeLists.txt`:
```cmake
# Set ThreadPool options (adjust as needed for your project)
set(BUILD_THREADPOOL_SHARED_LIBS OFF CACHE BOOL "Build ThreadPool as a static library" FORCE)
set(BUILD_THREADPOOL_SINGLETON ON CACHE BOOL "Enable ThreadPool singleton" FORCE)

# Append the parent's configuration request to the global list.
# This code checks if GLOBAL_THREADPOOL_REQUEST_LIST already has a value.
if(DEFINED GLOBAL_THREADPOOL_REQUEST_LIST AND NOT GLOBAL_THREADPOOL_REQUEST_LIST STREQUAL "")
    set(GLOBAL_THREADPOOL_REQUEST_LIST "${GLOBAL_THREADPOOL_REQUEST_LIST};${PROJECT_NAME}:1,1500000"
        CACHE INTERNAL "Global ThreadPool configuration requests" FORCE)
else()
    set(GLOBAL_THREADPOOL_REQUEST_LIST "${PROJECT_NAME}:1,1500000"
        CACHE INTERNAL "Global ThreadPool configuration requests" FORCE)
endif()

# Then add the ThreadPool subdirectory.
add_subdirectory(extern/ThreadPool)
```
#### Explanation:
- The snippet first sets some options for `ThreadPool`.
- It then checks if `GLOBAL_THREADPOOL_REQUEST_LIST` already exists and is non-empty.

   - If it exists, the new request (formatted as `${PROJECT_NAME}:1,1500000`) is appended using a semicolon (`;`) as a separator.
   - If it doesn't exist, the global variable is set with just the new request.

- Finally, it calls `add_subdirectory(extern/ThreadPool)` so that the `ThreadPool` CMake code can read and process the accumulated requests.

## Understanding the Unified Configuration
ThreadPool uses an internal function `_threadpool_unify_requests()` to merge all configuration requests into a single final configuration. Each request is recorded in the format:
```bash
<Caller>:<mode>,<tick>
```
The unification process works as follows:

- **Mode:**
    If any request specifies mode `1` (`PRIORITY`), then the final mode is set to 1. Otherwise, it remains `0` (`STANDARD`).

- **Tick:**
        If any request specifies a tick value of `0` (non‑adaptive), the final tick is set to `0`.
        Otherwise, the maximum tick value among all requests is chosen.

When unification occurs, the function prints all requests along with the final unified configuration. For example:
```bash
=== ThreadPool Requests ===
  Request: PROJECT_NAME:1,1500000
===========================
[PROJECT_NAME] Final Unified Configuration: Mode = PRIORITY, TICK = 1500000
```
### Singleton Behavior
If `BUILD_THREADPOOL_SINGLETON` is enabled, the unified configuration is applied to a singleton instance of `ThreadPoolManager`. This means:

- All submodules and parent projects share the same ThreadPool configuration.
- The final unified mode and tick values control the behavior of the singleton.
- This ensures consistent behavior across all projects that use ThreadPool.

## Usage
After installing and linking the `ThreadPool` library to your project, you can utilize it within your application as follows:

```cpp
#include "ThreadPool.hpp"

int main() {
    // Create a ThreadPool with 4 worker threads.
    ThreadPool pool(4);

    // Enqueue a task into the pool using a lambda function.
    auto result = pool.queue([]() -> int {
        // Your task logic...
        return 42;
    });

    // Get the result of the task execution.
    std::cout << "Result: " << result.get() << std::endl;

    return 0;
}
```

## Usage
```bash
    ninja test # ctest
```

## Contributing to ThreadPool
We welcome contributions to ThreadPool! Whether you're a seasoned developer or just starting out, there are many ways to contribute to this project.

### Ways to Contribute
1. Reporting bugs or problems: If you encounter any bugs or issues, please open an issue in the GitHub repository. We appreciate detailed reports that help us understand the cause of the problem.

2. Suggesting enhancements: Do you have ideas for making ThreadPool even better? We'd love to hear your suggestions. Please create an issue to discuss your ideas.

3. Code contributions: Want to get hands-on with the code? Great! Please feel free to fork the repository and submit your pull requests. Whether it's bug fixes, new features, or improvements to documentation, all contributions are greatly appreciated.

### Contribution Process
1. Fork the repository and create your branch from main.
2. If you've added or changed code, please ensure the testing suite passes. Add new tests for new features or bug fixes.
3. Ensure your code follows the existing style to keep the codebase consistent and readable.
4. Include detailed comments in your code so others can understand your changes.
5. Write a descriptive commit message.
6. Push your branch to GitHub and submit a pull request.