#include <gtest/gtest.h>
#include "ThreadTask.hpp"

TEST(ThreadTaskTest, ConstructorAndGetters) {
    ThreadPool::ThreadTask task([](){ return 1+1; }, 2, 3);
    ASSERT_EQ(task.get_priority(), 2);
    ASSERT_EQ(task.get_retries(), 3);
}

TEST(ThreadTaskTest, Execution) {
    ThreadPool::ThreadTask task([](){ return 42; }, 0, 0);
    task.execute();
    ASSERT_FALSE(task.done());
    std::future<std::any> fut = task.get_future();
    ASSERT_TRUE(task.done());
    ASSERT_EQ(std::any_cast<int>(fut.get()), 42);
}

TEST(ThreadTaskTest, RetryMechanism) {
    int execution_count = 0;
    ThreadPool::ThreadTask task([&execution_count](){
        execution_count++;
        throw std::runtime_error("Test");
    }, 0, 3);
    
    task.execute();
    
    ASSERT_EQ(execution_count, 3);
}

TEST(ThreadTaskTest, PriorityChange) {
    ThreadPool::ThreadTask task([](){ return 1; }, 2, 3);
    ASSERT_EQ(task.get_priority(), 2);
    task.increase_priority(3);
    ASSERT_EQ(task.get_priority(), 5);
}

TEST(ThreadTaskTest, RetryChange) {
    ThreadPool::ThreadTask task([](){ return 1; }, 2, 3);
    ASSERT_EQ(task.get_retries(), 3);
    task.increase_retries(2);
    ASSERT_EQ(task.get_retries(), 5);
}

TEST(ThreadTaskTest, Comparator) {
    ThreadPool::ThreadTask high_priority_task([](){ return 1; }, 4, 3);
    ThreadPool::ThreadTask low_priority_task([](){ return 1; }, 2, 3);
    
    ASSERT_TRUE(high_priority_task > low_priority_task);
    ASSERT_FALSE(low_priority_task > high_priority_task);
    ASSERT_FALSE(high_priority_task < low_priority_task);
    ASSERT_TRUE(low_priority_task < high_priority_task);
}
