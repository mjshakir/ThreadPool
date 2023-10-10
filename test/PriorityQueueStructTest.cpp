#include <gtest/gtest.h>
#include <iostream>
#include "PriorityQueue.hpp"

class CustomStruct {
public:
    CustomStruct(int val) : value(val) {}

    bool operator<(const CustomStruct &rhs) const {
        return value < rhs.value;
    }

    bool operator==(const CustomStruct &rhs) const {
        return value == rhs.value;
    }

    friend bool operator>=(const CustomStruct &lhs, const CustomStruct &rhs) {
        return lhs.value >= rhs.value;
    }

    friend std::ostream &operator<<(std::ostream &os, const CustomStruct &obj) {
        return os << "CustomStruct(value: " << obj.value << ")";
    }

private:
    int value;
};


struct CustomTask {
    int value;
    bool isDone;

    CustomTask(int val, bool done) : value(val), isDone(done) {}

    bool is_done() const { return isDone; }
    bool done() const { return isDone; }

    bool operator<(const CustomTask& other) const {
        return value < other.value;
    }

    bool operator==(const CustomTask& other) const {
        return value == other.value && isDone == other.isDone;
    }

    friend bool operator>=(const CustomTask& lhs, const CustomTask& rhs) {
        return lhs.value >= rhs.value;
    }

    friend std::ostream &operator<<(std::ostream &os, const CustomTask &obj) {
        return os << "CustomTask(value: " << obj.value << ", isDone: " << std::boolalpha << obj.isDone << ")";
    }
};


class PriorityQueueTest : public ::testing::Test {
protected:
    ThreadPool::PriorityQueue<CustomStruct> priorityQueue;
};

TEST_F(PriorityQueueTest, CustomStructOrdering) {
    priorityQueue.push(CustomStruct(3));
    priorityQueue.push(CustomStruct(1));
    priorityQueue.push(CustomStruct(2));

    EXPECT_FALSE(priorityQueue.empty());
    EXPECT_EQ(priorityQueue.size(), 3);

    auto topElement = priorityQueue.pop_top();
    ASSERT_TRUE(topElement.has_value());
    EXPECT_EQ(topElement.value(), CustomStruct(3));

    topElement = priorityQueue.pop_top();
    ASSERT_TRUE(topElement.has_value());
    EXPECT_EQ(topElement.value(), CustomStruct(2));

    topElement = priorityQueue.pop_top();
    ASSERT_TRUE(topElement.has_value());
    EXPECT_EQ(topElement.value(), CustomStruct(1));

    EXPECT_TRUE(priorityQueue.empty());
    EXPECT_EQ(priorityQueue.size(), 0);
}

class CustomStructPriorityQueueTest : public ::testing::Test {
protected:
    ThreadPool::PriorityQueue<CustomTask> priorityQueue;
};

TEST_F(CustomStructPriorityQueueTest, RemoveFunctionality) {
    priorityQueue.push(CustomTask(1, false));
    priorityQueue.push(CustomTask(2, true));
    priorityQueue.push(CustomTask(3, false));
    
    EXPECT_EQ(priorityQueue.size(), 3);
    
    priorityQueue.remove();
    
    EXPECT_EQ(priorityQueue.size(), 2);
    
    auto topElement = priorityQueue.pop_top();
    ASSERT_TRUE(topElement.has_value());
    EXPECT_EQ(topElement.value(), CustomTask(3, false));
    
    topElement = priorityQueue.pop_top();
    ASSERT_TRUE(topElement.has_value());
    EXPECT_EQ(topElement.value(), CustomTask(1, false));
    
    EXPECT_FALSE(topElement.value().is_done());
    EXPECT_FALSE(topElement.value().done());
}

TEST_F(PriorityQueueTest, StressTestCustomStruct) {
    constexpr int itemCount = 1000000;  // a large number of items

    priorityQueue.reserve(itemCount);
    
    for (int i = 0; i < itemCount; ++i) {
        priorityQueue.push(CustomStruct(rand()));  // using random values
    }

    CustomStruct prevValue(INT_MAX);  // initialize with the maximum possible int value
    while (!priorityQueue.empty()) {
        auto current = priorityQueue.pop_top();
        ASSERT_TRUE(current.has_value());
        ASSERT_GE(prevValue, current.value());
        prevValue = current.value();
    }
}

TEST_F(CustomStructPriorityQueueTest, StressTestCustomTask) {
    constexpr int itemCount = 1000000;  // a large number of items

    priorityQueue.reserve(itemCount);

    for (int i = 0; i < itemCount; ++i) {
        priorityQueue.push(CustomTask(rand(), rand() % 2 == 0));  // using random values and states
    }

    CustomTask prevValue(INT_MAX, true);  // initialize with maximum possible int value and true
    while (!priorityQueue.empty()) {
        auto current = priorityQueue.pop_top();
        ASSERT_TRUE(current.has_value());
        ASSERT_GE(prevValue, current.value());
        prevValue = current.value();
    }
}



int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
