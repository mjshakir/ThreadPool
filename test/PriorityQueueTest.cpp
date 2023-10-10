#include <gtest/gtest.h>
#include <thread>
#include "PriorityQueue.hpp"

class PriorityQueueTest : public ::testing::Test {
protected:
    ThreadPool::PriorityQueue<int> priorityQueue;
};

TEST_F(PriorityQueueTest, ConstructEmpty) {
    EXPECT_TRUE(priorityQueue.empty());
    EXPECT_EQ(priorityQueue.size(), 0);
}

TEST_F(PriorityQueueTest, PushPopElement) {
    priorityQueue.push(1);
    EXPECT_FALSE(priorityQueue.empty());
    EXPECT_EQ(priorityQueue.size(), 1);

    auto topElement = priorityQueue.pop_top();
    ASSERT_TRUE(topElement.has_value());
    EXPECT_EQ(topElement.value(), 1);
    EXPECT_TRUE(priorityQueue.empty());
    EXPECT_EQ(priorityQueue.size(), 0);
}

TEST_F(PriorityQueueTest, OrderElementsCorrectly) {
    priorityQueue.push(3);
    priorityQueue.push(1);
    priorityQueue.push(2);
    EXPECT_EQ(priorityQueue.size(), 3);

    EXPECT_EQ(priorityQueue.pop_top().value(), 3);
    EXPECT_EQ(priorityQueue.pop_top().value(), 2);
    EXPECT_EQ(priorityQueue.pop_top().value(), 1);
    EXPECT_TRUE(priorityQueue.empty());
}

TEST_F(PriorityQueueTest, EmplaceElement) {
    priorityQueue.emplace(1);
    EXPECT_FALSE(priorityQueue.empty());
    EXPECT_EQ(priorityQueue.size(), 1);

    EXPECT_EQ(priorityQueue.pop_top().value(), 1);
    EXPECT_TRUE(priorityQueue.empty());
}

TEST_F(PriorityQueueTest, RemoveElement) {
    priorityQueue.push(1);
    priorityQueue.push(2);
    priorityQueue.remove(1);
    
    EXPECT_FALSE(priorityQueue.empty());
    EXPECT_EQ(priorityQueue.size(), 1);
    
    EXPECT_EQ(priorityQueue.pop_top().value(), 2);
    EXPECT_TRUE(priorityQueue.empty());
}

TEST_F(PriorityQueueTest, TopElement) {
    priorityQueue.push(3);
    priorityQueue.push(1);

    auto topElement = priorityQueue.top();
    ASSERT_TRUE(topElement.has_value());
    EXPECT_EQ(topElement.value(), 3);
    EXPECT_EQ(priorityQueue.size(), 2); // Ensure size hasn't changed after top()
}

TEST_F(PriorityQueueTest, StressTest) {
    constexpr size_t numThreads = 10;
    constexpr size_t numIterations = 1000;

    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    priorityQueue.reserve(numIterations);

    std::atomic_size_t popCounter = 0;

    // Launch numThreads threads...
    for (size_t i = 0; i < numThreads; ++i) {
        threads.push_back(std::thread([&, i]() {
            for (size_t j = 0; j < numIterations; ++j) {
                // Odd threads push elements to the queue
                if (i % 2 == 1) {
                    priorityQueue.push(static_cast<int>(j + i * numIterations));
                }
                // Even threads pop elements from the queue
                else {
                    auto item = priorityQueue.pop_top();
                    if (item.has_value()) {
                        ++popCounter;
                    }
                }
            }
        }));
    }

    // Join all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Log some statistics
    std::cout << "Total popped items: " << popCounter.load() << std::endl;
    std::cout << "Final queue size: " << priorityQueue.size() << std::endl;

    // Check that queue state is valid (e.g., size and ordering)
    int prevValue = INT_MAX;
    size_t size = priorityQueue.size();
    for (size_t i = 0; i < size; ++i) {
        auto item = priorityQueue.pop_top();
        ASSERT_TRUE(item.has_value());
        ASSERT_GE(prevValue, item.value());
        prevValue = item.value();
    }
    EXPECT_EQ(priorityQueue.size(), 0);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
