#include <thread>
#include <random>
#include <chrono>

#include <benchmark/benchmark.h>

#include "ThreadTask.hpp"

using namespace ThreadPool;

// Utility function to simulate work
int simulate_work(int value) {
    std::this_thread::sleep_for(std::chrono::nanoseconds(10));
    return value * 2;
}

static void BM_ThreadTask_Execute(benchmark::State& state) {
    for (auto _ : state) {
        ThreadTask task([=]() { return simulate_work(42); }, 0U, 2U);
        task.execute();
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ThreadTask_Execute)->RangeMultiplier(10)->Range(1, 10000)->Complexity(benchmark::oAuto);

static void BM_ThreadTask_TryExecute(benchmark::State& state) {
    for (auto _ : state) {
        ThreadTask task([=]() { return simulate_work(42); }, 0U, 2U);
        bool success = task.try_execute();
        benchmark::DoNotOptimize(success);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ThreadTask_TryExecute)->RangeMultiplier(10)->Range(1, 10000)->Complexity(benchmark::oAuto);

static void BM_ThreadTask_GetFuture(benchmark::State& state) {
    for (auto _ : state) {
        ThreadTask task([=]() { return simulate_work(42); }, 0U, 2U);
        task.execute();
        auto future = task.get_future();
        benchmark::DoNotOptimize(future.get());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ThreadTask_GetFuture)->RangeMultiplier(10)->Range(1, 10000)->Complexity(benchmark::oAuto);

static void BM_ThreadTask_IncreaseRetries(benchmark::State& state) {
    ThreadTask task([=]() { return simulate_work(42); }, 0U, 2U);
    for (auto _ : state) {
        task.increase_retries(1);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ThreadTask_IncreaseRetries)->RangeMultiplier(10)->Range(1, 10000)->Complexity(benchmark::oAuto);

static void BM_ThreadTask_DecreaseRetries(benchmark::State& state) {
    ThreadTask task([=]() { return simulate_work(42); }, 0U, 10U);
    for (auto _ : state) {
        task.decrease_retries(1);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ThreadTask_DecreaseRetries)->RangeMultiplier(10)->Range(1, 10000)->Complexity(benchmark::oAuto);

static void BM_ThreadTask_IncreasePriority(benchmark::State& state) {
    ThreadTask task([=]() { return simulate_work(42); }, 0U, 2U);
    for (auto _ : state) {
        task.increase_priority(1);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ThreadTask_IncreasePriority)->RangeMultiplier(10)->Range(1, 10000)->Complexity(benchmark::oAuto);

static void BM_ThreadTask_DecreasePriority(benchmark::State& state) {
    ThreadTask task([=]() { return simulate_work(42); }, 10U, 2U);
    for (auto _ : state) {
        task.decrease_priority(1);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ThreadTask_DecreasePriority)->RangeMultiplier(10)->Range(1, 10000)->Complexity(benchmark::oAuto);

static void BM_ThreadTask_GetStatus(benchmark::State& state) {
    ThreadTask task([=]() { return simulate_work(42); }, 0U, 2U);
    for (auto _ : state) {
        uint8_t status = task.get_status();
        benchmark::DoNotOptimize(status);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ThreadTask_GetStatus)->RangeMultiplier(10)->Range(1, 10000)->Complexity(benchmark::oAuto);

BENCHMARK_MAIN();