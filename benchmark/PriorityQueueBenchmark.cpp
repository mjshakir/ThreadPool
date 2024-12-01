#include <random>
#include <vector>
#include <algorithm>

#include <benchmark/benchmark.h>
#include "PriorityQueue.hpp"

using namespace ThreadPool;

// Utility function to generate a random vector of integers
std::vector<int> generate_random_vector(size_t size, int min = 0, int max = 1000000) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    
    std::vector<int> vec(size);
    std::generate(vec.begin(), vec.end(), [&]() { return dis(gen); });
    return vec;
}

static void BM_PriorityQueue_Push(benchmark::State& state) {
    PriorityQueue<int> pq;
    auto random_values = generate_random_vector(state.range(0));
    for (auto _ : state) {
        for (const auto& value : random_values) {
            pq.push(value);
        }
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_PriorityQueue_Push)->RangeMultiplier(10)->Range(1, 10000)->Complexity(benchmark::oAuto);

static void BM_PriorityQueue_Emplace(benchmark::State& state) {
    PriorityQueue<int> pq;
    auto random_values = generate_random_vector(state.range(0));
    for (auto _ : state) {
        for (const auto& value : random_values) {
            pq.emplace(value);
        }
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_PriorityQueue_Emplace)->RangeMultiplier(10)->Range(1, 10000)->Complexity(benchmark::oAuto);

static void BM_PriorityQueue_Top(benchmark::State& state) {
    PriorityQueue<int> pq;
    auto random_values = generate_random_vector(state.range(0));
    for (const auto& value : random_values) {
        pq.push(value);
    }
    for (auto _ : state) {
        auto top = pq.top();
        benchmark::DoNotOptimize(top);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_PriorityQueue_Top)->RangeMultiplier(10)->Range(1, 10000)->Complexity(benchmark::oAuto);

static void BM_PriorityQueue_Pop(benchmark::State& state) {
    PriorityQueue<int> pq;
    auto random_values = generate_random_vector(state.range(0));
    for (const auto& value : random_values) {
        pq.push(value);
    }
    for (auto _ : state) {
        if (!pq.empty()) {
            pq.pop();
        }
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_PriorityQueue_Pop)->RangeMultiplier(10)->Range(1, 10000)->Complexity(benchmark::oAuto);

static void BM_PriorityQueue_Size(benchmark::State& state) {
    PriorityQueue<int> pq;
    auto random_values = generate_random_vector(state.range(0));
    for (const auto& value : random_values) {
        pq.push(value);
    }
    for (auto _ : state) {
        auto size = pq.size();
        benchmark::DoNotOptimize(size);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_PriorityQueue_Size)->RangeMultiplier(10)->Range(1, 10000)->Complexity(benchmark::oAuto);

static void BM_PriorityQueue_RemoveTask(benchmark::State& state) {
    PriorityQueue<int> pq;
    auto random_values = generate_random_vector(state.range(0));
    for (const auto& value : random_values) {
        pq.push(value);
    }
    for (auto _ : state) {
        if (!pq.empty()) {
            pq.remove(random_values[state.range(0) / 2]);
        }
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_PriorityQueue_RemoveTask)->RangeMultiplier(10)->Range(1, 10000)->Complexity(benchmark::oAuto);

BENCHMARK_MAIN();
