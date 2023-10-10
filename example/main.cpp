#include <iostream>
#include <chrono>
#include <cmath>
#include "ThreadPool.hpp"

constexpr size_t _size = 10UL;
constexpr size_t TASKS = 10000UL;
constexpr size_t ITERATIONS = 100000UL;

// Define a computationally intensive task.
double complexTask(size_t iteration) {
    double result = 0.0;
    for (size_t i = 0; i < iteration; ++i) {
        result += std::pow(std::sin(i) * std::cos(i), 2);
    }
    return result;
}// end double complexTask(int iteration)

// Function to run the test without thread pool
double runWithoutThreadPool(void) {
    auto start = std::chrono::high_resolution_clock::now();
    double results = 0.;
    for (size_t i = 0; i < TASKS; ++i) {
        results += complexTask(ITERATIONS);
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}// end double runWithoutThreadPool(void)

// Function to run the test with thread pool
double runWithThreadPool(void) {
    ThreadPool::ThreadPool _threads(TASKS);
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::future<double>> _f_results;
    _f_results.reserve(TASKS);
    for (size_t i = 0; i < TASKS; ++i) {
        _f_results.emplace_back(_threads.queue(true, complexTask, ITERATIONS).get_future());
    }
    double results = 0.;
    for(auto& _f_result : _f_results) {
        results += _f_result.get();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}// end double runWithThreadPool(void)

int main(void)
{
    

    {
        ThreadPool::ThreadPool _threads(_size);
        std::vector<std::future<int>> results;
        results.reserve(_size);
        for (int i = 0; i < _size; ++i) {
            results.emplace_back(_threads.queue(true, [](int value) { return value * value; }, i).get_future());
        }

        for(auto& result : results){
            std::cout << "Return Value :[" << result.get() << "]" << std::endl;
        }
    }
    {
        ThreadPool::ThreadPool _threads(_size);
        for (size_t i = 0; i < _size; ++i) {
            _threads.queue(true, [](int value) { std::cout << "Print Value:[" << value << "]" << std::endl; }, i).set_priority(_size-i);
        }
    }
    {
        const auto noThreadPoolDuration = runWithoutThreadPool();
        std::cout << "Time without ThreadPool: " << noThreadPoolDuration << "ms" << std::endl;

        const auto withThreadPoolDuration = runWithThreadPool();
        std::cout << "Time with ThreadPool: " << withThreadPoolDuration << "ms" << std::endl;

        double overhead = withThreadPoolDuration / noThreadPoolDuration;
        std::cout << "Overhead: " << overhead << "x" << std::endl;

    }
    return 0;
}
