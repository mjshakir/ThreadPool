# ThreadPool

ThreadPool is a high-performance C++ library designed for efficient parallel execution of tasks, utilizing a pool of managed threads. Under the hood, it leverages the power of `std::jthread` introduced in C++20, which automatically manages the life cycle of threads, saving developers from the intricacies of explicit thread management.

This library stands out by offering flexibility in task scheduling through either a standard Deque or a custom-implemented PriorityQueue. This feature provides refined control over task execution order, allowing priority-based task handling. Whether you are dealing with simple parallel tasks or need advanced control over task execution order, ThreadPool can cater to those requirements with ease and efficiency.

## Key Features

- Modern C++ implementation utilizing `std::jthread` for automatic thread management and reduced overhead in resource handling.
- Flexible task scheduling mechanisms including standard Deque or priority-based PriorityQueue, enabling refined control over task execution based on user-defined priorities.
- Simplified integration with existing CMake projects, making it straightforward to include in your project.
- Easy management of a pool of threads.
- Task execution using either a `Deque` or a `PriorityQueue`.
- Customizable task prioritization (when using the `PriorityQueue`).
- Support for C++20.
- Easy integration with CMake projects.
- Options for both shared and static library build configurations to suit various application needs.

## Prerequisites
- CMake 3.14 or higher
- A compatible C++20 compiler

## How to Include ThreadPool in Your Project

You can easily include ThreadPool into your CMake project. Here is a step-by-step guide:

1. Add ThreadPool as a submodule to your project git repository.
    ```bash
    git submodule add https://github.com/mjshakir/ThreadPool.git extern/ThreadPool
    ```

2. Add the following lines to your CMakeLists.txt file.
    ```cmake
    add_subdirectory(extern/ThreadPool)

    # Link your application with ThreadPool
    target_link_libraries(your_target_name PRIVATE ThreadPool)
    ```

### Choosing Library Type (Static or Shared)

By default, ThreadPool is built as a shared library. If you prefer a static library, you can set the `BUILD_THREADPOOL_SHARED_LIBS` option to `OFF` when generating the build system files:

```bash
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