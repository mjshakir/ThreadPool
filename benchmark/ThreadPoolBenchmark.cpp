#include <thread>
#include <vector>
#include <future>
#include <chrono>
#include <benchmark/benchmark.h>
#include "ThreadPool.hpp"

// Utility function to simulate work
int simulate_work(int value) {
    std::this_thread::sleep_for(std::chrono::nanoseconds(10)); // Reduced sleep time to avoid prolonged blocking
    return value * 2;
}

// Benchmark for ThreadPool constructor and destructor
static void BM_ThreadPool_Constructor(benchmark::State& state) {
    for (auto _ : state) {
        ThreadPool::ThreadPool<ThreadPool::ThreadMode::STANDARD> pool(state.range(0));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ThreadPool_Constructor)->RangeMultiplier(2)->Range(4, 64)->Complexity(benchmark::oAuto);

// Benchmark for queueing tasks
static void BM_ThreadPool_QueueTask(benchmark::State& state) {
    ThreadPool::ThreadPool<ThreadPool::ThreadMode::STANDARD> pool(state.range(0));
    for (auto _ : state) {
        auto future = pool.queue(simulate_work, 42);
        benchmark::DoNotOptimize(future);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ThreadPool_QueueTask)->RangeMultiplier(2)->Range(4, 64)->Complexity(benchmark::oAuto);

// Benchmark for executing tasks
static void BM_ThreadPool_ExecuteTask(benchmark::State& state) {
    ThreadPool::ThreadPool<ThreadPool::ThreadMode::STANDARD> pool(state.range(0));
    for (auto _ : state) {
        auto future = pool.queue(simulate_work, 42);
        future.wait();
        benchmark::DoNotOptimize(future.get());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ThreadPool_ExecuteTask)->RangeMultiplier(2)->Range(4, 64)->Complexity(benchmark::oAuto);

// Benchmark for enqueueing void tasks (asynchronous fire-and-forget)
static void BM_ThreadPool_VoidAsync(benchmark::State& state) {
    ThreadPool::ThreadPool<ThreadPool::ThreadMode::STANDARD> pool(state.range(0));
    for (auto _ : state) {
        pool.queue([]() {});
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ThreadPool_VoidAsync)->RangeMultiplier(2)->Range(4, 64)->Complexity(benchmark::oAuto);

// Benchmark for enqueueing void tasks (synchronous with future)
static void BM_ThreadPool_VoidSync(benchmark::State& state) {
    ThreadPool::ThreadPool<ThreadPool::ThreadMode::STANDARD> pool(state.range(0));
    for (auto _ : state) {
        auto fut = pool.queue<ThreadPool::ThreadSynchronization::SYNCHRONOUS>([]() {});
        benchmark::DoNotOptimize(fut.valid());
        fut.wait();
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ThreadPool_VoidSync)->RangeMultiplier(2)->Range(4, 64)->Complexity(benchmark::oAuto);

// Benchmark for handling a burst of tasks
static void BM_ThreadPool_BurstTasks(benchmark::State& state) {
    ThreadPool::ThreadPool<ThreadPool::ThreadMode::STANDARD> pool(state.range(0));
    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        for (int i = 0; i < state.range(1); ++i) {
            futures.push_back(pool.queue(simulate_work, i));
        }
        for (auto& future : futures) {
            benchmark::DoNotOptimize(future.get());
        }
    }
    state.SetComplexityN(state.range(0) * state.range(1));
}
BENCHMARK(BM_ThreadPool_BurstTasks)->Ranges({{4, 64}, {10, 1000}})->Complexity(benchmark::oAuto);

// Benchmark for priority-based task execution
static void BM_ThreadPool_PriorityQueueTask(benchmark::State& state) {
    ThreadPool::ThreadPool<ThreadPool::ThreadMode::PRIORITY> pool(state.range(0));
    for (auto _ : state) {
        auto task = pool.queue(true, simulate_work, 42);
        task.set_priority(10);
        benchmark::DoNotOptimize(task.get_future());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ThreadPool_PriorityQueueTask)->RangeMultiplier(2)->Range(4, 64)->Complexity(benchmark::oAuto);

// Benchmark for enum-to-string conversions
static void BM_ThreadMode_NameLookup(benchmark::State& state) {
    const auto mode = state.range(0) == 0 ? ThreadPool::ThreadMode::STANDARD : ThreadPool::ThreadMode::PRIORITY;
    for (auto _ : state) {
        benchmark::DoNotOptimize(ThreadPool::ThreadMode_name(mode));
    }
    state.SetComplexityN(1);
}
BENCHMARK(BM_ThreadMode_NameLookup)->DenseRange(0, 1);

static void BM_ThreadSynchronization_NameLookup(benchmark::State& state) {
    const auto sync_mode = state.range(0) == 0 ? ThreadPool::ThreadSynchronization::ASYNCHRONOUS : ThreadPool::ThreadSynchronization::SYNCHRONOUS;
    for (auto _ : state) {
        benchmark::DoNotOptimize(ThreadPool::ThreadSynchronization_name(sync_mode));
    }
    state.SetComplexityN(1);
}
BENCHMARK(BM_ThreadSynchronization_NameLookup)->DenseRange(0, 1);

BENCHMARK_MAIN();

/*
Time Complexity Analysis:

1. ThreadPool Constructor/Destructor:
   - Complexity: O(1) per thread, as the number of threads created is fixed by the input range.
   - Reasoning: The constructor creates the worker threads, and the destructor joins them.

2. Queueing a Task:
   - Complexity: O(1) per task.
   - Reasoning: Adding a task to the queue is a constant time operation.

3. Executing a Task:
   - Complexity: O(1) per task.
   - Reasoning: Each task execution is independent, so adding and retrieving a task is O(1).

4. Handling a Burst of Tasks:
   - Complexity: O(N * M), where N is the number of threads and M is the number of tasks per burst.
   - Reasoning: Each thread processes multiple tasks, and each task retrieval and execution takes O(1).

5. Priority Queue Task Execution:
   - Complexity: O(log N) for adding a task, where N is the number of tasks in the priority queue.
   - Reasoning: PriorityQueue uses a heap, and adding/removing elements takes O(log N) time.
*/
