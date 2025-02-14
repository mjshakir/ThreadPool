#include <iostream>
#include <chrono>
#include <cmath>
#include "ThreadPool.hpp"
#if THREADPOOL_ENABLE_SINGLETON
    #include "ThreadPoolManager.hpp"
#endif

constexpr size_t _size = 10UL;
constexpr size_t TASKS = 10000UL;
constexpr size_t ITERATIONS = 100000UL;

#if THREADPOOL_ENABLE_SINGLETON
    void runThreadPoolManager(void) {
        auto& _thread_pool = ThreadPool::ThreadPoolManager::get_instance(_size).get_thread_pool();

        std::cout   << "Testing ThreadPoolManager Singleton"
                    << " | ThreadMode mode: " << ThreadPool::ThreadMode_name(_thread_pool.mode()) 
                    << " | Adoptive: " << std::boolalpha << _thread_pool.adoptive()
                    << " | Adoptive Tick: " << _thread_pool.adoptive_tick_size() << std::endl;

    }// end void runThreadPoolManager(void)
#endif
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
    ThreadPool::ThreadPool<ThreadPool::ThreadMode::PRIORITY> _threads(TASKS);
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

double runWithThreadPoolDeque(void) {
    ThreadPool::ThreadPool _threads(TASKS);
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::future<double>> _f_results;
    _f_results.reserve(TASKS);
    for (size_t i = 0; i < TASKS; ++i) {
        _f_results.emplace_back(_threads.queue(complexTask, ITERATIONS));
    }
    double results = 0.;
    for(auto& _f_result : _f_results) {
        results += _f_result.get();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}// end double runWithThreadPoolDeque(void)

int main(void) {

#if THREADPOOL_ENABLE_SINGLETON
    {
        runThreadPoolManager();
    }
#endif
    {
        ThreadPool::ThreadPool<ThreadPool::ThreadMode::STANDARD> _threads(_size);
        std::vector<std::future<int>> results;
        results.reserve(_size);
        for (int i = 0; i < _size; ++i) {
            results.emplace_back(_threads.queue([](int value) { return value * value; }, i));
        }

        for(auto& result : results){
            std::cout << "ThreadMode::STANDARD Future Return Value:[" << result.get() << "] \n";
        }
        std::cout << std::flush;
    }
    {
        ThreadPool::ThreadPool<ThreadPool::ThreadMode::STANDARD> _threads(_size);
        for (size_t i = 0; i < _size; ++i) {
            _threads.queue([](int value) { std::cout << "ThreadMode::STANDARD Print Value:[" << value << "]" << std::endl; }, i);
        }
    }
    {
        ThreadPool::ThreadPool<ThreadPool::ThreadMode::PRIORITY> _threads(_size);
        std::vector<std::future<int>> results;
        results.reserve(_size);
        for (int i = 0; i < _size; ++i) {
            results.emplace_back(_threads.queue(true, [](int value) { return value * value; }, i).get_future());
        }

        for(auto& result : results){
            std::cout << "ThreadMode::PRIORITY Future Return Value:[" << result.get() << "]\n";
        }
        std::cout << std::flush;
    }
    {
        ThreadPool::ThreadPool<ThreadPool::ThreadMode::PRIORITY> _threads(_size);
        for (size_t i = 0; i < _size; ++i) {
            _threads.queue(true, [](int value) { std::cout << "ThreadMode::PRIORITY Print Value:[" << value << "]" << std::endl; }, i).set_priority(_size-i);
        }
    }
    {
        ThreadPool::ThreadPool<ThreadPool::ThreadMode::PRIORITY> _threads(_size);
        std::vector<std::future<int>> results;
        results.reserve(_size);
        for (int i = 0; i < _size; ++i) {
            results.emplace_back(_threads.queue([](int value) { return value * value; }, i).get_future());
        }

        for(auto& result : results){
            std::cout << "ThreadMode::PRIORITY Future Return Value:[" << result.get() << "]\n";
        }
        std::cout << std::flush;
    }
    {
        ThreadPool::ThreadPool<ThreadPool::ThreadMode::PRIORITY> _threads(_size);
        for (size_t i = 0; i < _size; ++i) {
            _threads.queue([](int value) { std::cout << "ThreadMode::PRIORITY Print Value:[" << value << "]" << std::endl; }, i).set_priority(_size-i);
        }
    }
    {
        
        std::cout << "Running complex task with ThreadPool and without ThreadPool using ThreadMode::STANDARD " << std::endl;

        const auto noThreadPoolDuration = runWithoutThreadPool();
        std::cout << "Time without ThreadPool: " << noThreadPoolDuration << "ms" << std::endl;

        const auto withThreadPoolDuration = runWithThreadPoolDeque();
        std::cout << "Time with ThreadPool Deque: " << withThreadPoolDuration << "ms" << std::endl;

        double overhead = withThreadPoolDuration / noThreadPoolDuration;
        std::cout << "Overhead: " << overhead << "x" << std::endl;

    }
    {

        std::cout << "Running complex task with ThreadPool and without ThreadPool using ThreadMode::PRIORITY " << std::endl;

        const auto noThreadPoolDuration = runWithoutThreadPool();
        std::cout << "Time without ThreadPool: " << noThreadPoolDuration << "ms" << std::endl;

        const auto withThreadPoolDuration = runWithThreadPool();
        std::cout << "Time with ThreadPool PriorityQueue: " << withThreadPoolDuration << "ms" << std::endl;

        double overhead = withThreadPoolDuration / noThreadPoolDuration;
        std::cout << "Overhead: " << overhead << "x" << std::endl;

    }
    return 0;
}