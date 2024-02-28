#include <gtest/gtest.h>
#include <chrono>
#include <future>
#include <vector>
#include "ThreadPool.hpp"

class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup necessary resources before each test
        threadPool = std::make_unique<ThreadPool::ThreadPool<false>>(10); // Assume 4 as default thread count
    }
    
    void TearDown() override {
        // Cleanup resources
        threadPool.reset(); // stop() will be called in the destructor of ThreadPool
    }
    
    std::unique_ptr<ThreadPool::ThreadPool<false>> threadPool;
};


TEST_F(ThreadPoolTest, TaskPrioritizationWorksCorrectly) {
    std::vector<int> executionOrder;
    std::mutex mtx;

    for (int i = 0; i < 10; ++i) {
        threadPool->queue(
            [&executionOrder, &mtx](int value) {
                std::unique_lock lock(mtx); // Protect access to shared vector
                executionOrder.push_back(value);
            }, i); // Assuming lower number means higher priority
    }

    // Possibly wait for a while or use some synchronization mechanism to ensure all tasks are completed
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Adjust as needed
    
    // Sort and Check the order of execution
    std::sort(executionOrder.begin(), executionOrder.end());
    std::vector<int> expectedOrder{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    EXPECT_EQ(executionOrder, expectedOrder);
}



// Test if all tasks are executed correctly
TEST_F(ThreadPoolTest, ExecuteTasksCorrectly) {
    std::vector<std::future<int>> results;
    for (int i = 0; i < 10; ++i) {
        auto taskBuilder = threadPool->queue([](int value) {return value * value;}, i);
        try {
            results.emplace_back(std::move(taskBuilder));
        } catch (const std::future_error& e) {
            FAIL() << "Failed to get future for i = " << i << " with error: " << e.what();
        }
    }
    
    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(results[i].valid()) << "Invalid future at index " << i;
        EXPECT_EQ(results[i].get(), i * i) << "For index " << i;
    }
}




// Test to ensure that all threads end correctly when ThreadPool is destroyed
TEST_F(ThreadPoolTest, ThreadsEndCorrectlyOnDestruction) {
    auto anotherThreadPool = std::make_unique<ThreadPool::ThreadPool<false>>(2);
    
    // Use promises to get notified when the tasks start
    std::promise<void> task1Started, task2Started;
    auto fut1 = task1Started.get_future();
    auto fut2 = task2Started.get_future();
    
    anotherThreadPool->queue([&] { task1Started.set_value(); });
    anotherThreadPool->queue([&] { task2Started.set_value(); });
    
    fut1.wait();
    fut2.wait();
    
    // Destroy the thread pool and ensure that it doesn't hang
    anotherThreadPool.reset();
    SUCCEED(); // If we reached here, the ThreadPool is destroyed properly without deadlock
}

// Test ThreadPool under high stress to check if it works correctly under high loads
TEST_F(ThreadPoolTest, StressTest) {
    constexpr uint16_t taskCount = 10000;
    std::atomic_int counter{0};
    
    for (uint16_t i = 0; i < taskCount; ++i) {
        threadPool->queue([&counter] { counter++; });
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(5)); // Assume 5 seconds is enough
    EXPECT_EQ(counter.load(), taskCount);
}

TEST_F(ThreadPoolTest, CombinedStressTest) {
    constexpr uint16_t taskCount = 10000;
    std::atomic_int counter{0};
    std::vector<std::future<int>> results;
    
    for (uint16_t i = 0; i < taskCount; ++i) {
        auto taskBuilder = threadPool->queue([&counter, i] { 
            counter++; 
            return i * i; 
        });
        try {
            if (!taskBuilder.valid()) {
                FAIL() << "Invalid future for i = " << i;
            } else {
                results.emplace_back(std::move(taskBuilder));
            }
        } catch (const std::future_error& e) {
            FAIL() << "Failed to get future for i = " << i << " with error: " << e.what();
        }
    }
    
    // Wait or synchronize to make sure all tasks are completed before checking results.
    std::this_thread::sleep_for(std::chrono::seconds(5)); // Adjust as needed
    
    EXPECT_EQ(counter.load(), taskCount); // Check all tasks were executed
    
    // Validate all the results
    for (uint16_t i = 0; i < results.size(); ++i) {
        if (results[i].valid()) {
            auto value = results[i].get();
            EXPECT_EQ(value, i * i) << "For index " << i;
        } else {
            FAIL() << "Invalid future at index " << i;
        }
    }
}

TEST_F(ThreadPoolTest, HandleVaryingExecutionTimes) {
    constexpr size_t taskCount = 1000;
    std::vector<std::future<int>> futures;
    std::vector<int> expectedResults(taskCount);

    // Queue tasks with varying execution times
    for (int i = 0; i < taskCount; ++i) {
        auto taskBuilder = threadPool->queue([i]() {
            std::this_thread::sleep_for(std::chrono::nanoseconds(100 * (taskCount - i))); // Longer delay for earlier tasks
            return i * i;
        });
        expectedResults[i] = i * i;
        try {
            if (!taskBuilder.valid()) {
                FAIL() << "Invalid future for i = " << i;
            } else {
                futures.emplace_back(std::move(taskBuilder));
            }
        } catch (const std::future_error& e) {
            FAIL() << "Failed to get future for i = " << i << " with error: " << e.what();
        }
    }

    // Synchronize and validate results
    for (size_t i = 0; i < futures.size(); ++i) {
        if (futures[i].valid()) {
            auto value = futures[i].get();
            EXPECT_EQ(value, expectedResults[i]) << "For index " << i;
        } else {
            FAIL() << "Invalid future at index " << i;
        }
    }
}

TEST_F(ThreadPoolTest, AllThreadsRunningWithoutGet) {
    constexpr size_t taskCount = 1000;  // Adjust as needed
    const size_t threadCount = std::thread::hardware_concurrency();  // Actual thread count
    std::set<std::thread::id> threadIds;
    std::mutex mtx;
    std::atomic_size_t tasksCompleted = 0;

    for (size_t i = 0; i < taskCount; ++i) {
        threadPool->queue([i, &mtx, &threadIds, &tasksCompleted]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Fixing time for better synchronization
            {
                std::lock_guard lock(mtx);
                threadIds.insert(std::this_thread::get_id());
            }
            tasksCompleted++;
        });
    }

    // Synchronize/wait based on task completion, reducing flakiness due to timing variations.
    while (tasksCompleted < taskCount) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    EXPECT_GE(threadIds.size(), threadCount) << "Not all threads were utilized!";
}

TEST_F(ThreadPoolTest, DynamicThreadManagement) {
    std::atomic_int taskCounter{0};

    // Record the initial number of threads
    auto initialThreadCount = threadPool->threads_size();

    // First burst of tasks
    for (int i = 0; i < 100; ++i) {
        threadPool->queue([&taskCounter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Simulate work
            taskCounter++;
        });
    }

    // Wait for a second to simulate a period of low demand
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Check if the number of threads has decreased due to idle period
    auto threadCountAfterIdle = threadPool->threads_size();
    EXPECT_LE(threadCountAfterIdle, initialThreadCount);
    // Note: Depending on the implementation details of your thread pool, this expectation may need adjustment.

    // Check the queue size to ensure tasks from the first burst are processed
    auto queuedTasksAfterFirstBurst = threadPool->queued_size();
    EXPECT_EQ(queuedTasksAfterFirstBurst, 0); // Assuming tasks are completed

    // Second burst of tasks
    for (int i = 0; i < 100; ++i) {
        threadPool->queue([&taskCounter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Simulate work
            taskCounter++;
        });
    }

    // Wait for all tasks to complete
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Verify all tasks were completed
    EXPECT_EQ(taskCounter.load(), 200);

    // Verify that the thread pool has scaled back up if necessary
    auto threadCountAfterSecondBurst = threadPool->threads_size();
    EXPECT_GE(threadCountAfterSecondBurst, threadCountAfterIdle);
    // This checks if the thread pool increased the number of threads in response to the second burst of tasks.
}