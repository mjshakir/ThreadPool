#include <gtest/gtest.h>
#include "ThreadPoolManager.hpp"

//--------------------------------------------------------------
// Test Fixture
//--------------------------------------------------------------
class ThreadPoolManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset the ThreadPoolManager state before each test (if necessary)
        manager = &ThreadPool::ThreadPoolManager::get_instance();
    }

    void TearDown() override {
        // No specific teardown required since get_instance is a singleton
    }

    ThreadPool::ThreadPoolManager* manager;
};

//--------------------------------------------------------------
// Test Cases
//--------------------------------------------------------------

TEST_F(ThreadPoolManagerTest, SingletonInstance) {
    // Ensure ThreadPoolManager always returns the same instance
    ThreadPool::ThreadPoolManager& instance1 = ThreadPool::ThreadPoolManager::get_instance();
    ThreadPool::ThreadPoolManager& instance2 = ThreadPool::ThreadPoolManager::get_instance();
    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(ThreadPoolManagerTest, InitializeThreadPool) {
    // Configure the thread pool and ensure it's properly initialized
    bool configured = manager->configure<ThreadPool::ThreadMode::STANDARD, 0, ThreadPool::PrecedenceLevel::HIGH>();
    EXPECT_TRUE(configured);
    EXPECT_TRUE(manager->initialized());
}

TEST_F(ThreadPoolManagerTest, ReconfigureWithHigherPrecedence) {
    // Configure with low precedence first
    bool configuredLow = manager->configure<ThreadPool::ThreadMode::STANDARD, 1000000, ThreadPool::PrecedenceLevel::LOW>();
    EXPECT_TRUE(configuredLow);

    // Attempt to reconfigure with higher precedence
    bool configuredHigh = manager->configure<ThreadPool::ThreadMode::PRIORITY, 0, ThreadPool::PrecedenceLevel::HIGH>();
    EXPECT_TRUE(configuredHigh);

    // Ensure the higher precedence configuration succeeded
    EXPECT_TRUE(manager->initialized());
}

TEST_F(ThreadPoolManagerTest, ReconfigureWithLowerPrecedence) {
    // Configure with high precedence first
    bool configuredHigh = manager->configure<ThreadPool::ThreadMode::PRIORITY, 0, ThreadPool::PrecedenceLevel::HIGH>();
    EXPECT_TRUE(configuredHigh);

    // Attempt to reconfigure with lower precedence
    bool configuredLow = manager->configure<ThreadPool::ThreadMode::STANDARD, 1000000, ThreadPool::PrecedenceLevel::LOW>();
    EXPECT_FALSE(configuredLow);

    // Ensure the configuration remains unchanged
    EXPECT_TRUE(manager->initialized());
}

TEST_F(ThreadPoolManagerTest, GetThreadPoolFallback) {
    // Ensure get_thread_pool provides a fallback when uninitialized
    ThreadPool::ThreadPoolManager& uninitializedManager = ThreadPool::ThreadPoolManager::get_instance();
    EXPECT_FALSE(uninitializedManager.initialized());

    ThreadPool::ThreadPool<>& pool = uninitializedManager.get_thread_pool();
    EXPECT_NE(&pool, nullptr); // Default pool should be provided
}

TEST_F(ThreadPoolManagerTest, AdaptiveTickOverridesNonAdaptive) {
    // Configure with adaptive tick first
    bool configuredAdaptive = manager->configure<ThreadPool::ThreadMode::STANDARD, 1000000, ThreadPool::PrecedenceLevel::MEDIUM>();
    EXPECT_TRUE(configuredAdaptive);

    // Attempt to reconfigure with non-adaptive tick
    bool configuredNonAdaptive = manager->configure<ThreadPool::ThreadMode::STANDARD, 0, ThreadPool::PrecedenceLevel::MEDIUM>();
    EXPECT_TRUE(configuredNonAdaptive);
}

TEST_F(ThreadPoolManagerTest, FaultToleranceInvalidConfiguration) {
    // Attempt to retrieve the thread pool without configuration
    ThreadPool::ThreadPoolManager& uninitializedManager = ThreadPool::ThreadPoolManager::get_instance();

    EXPECT_THROW(uninitializedManager.get_thread_pool(), std::logic_error);
}

TEST_F(ThreadPoolManagerTest, MultithreadedInitialization) {
    // Ensure ThreadPoolManager handles concurrent initialization safely
    bool configured1 = false, configured2 = false;

    auto thread1 = std::thread([&]() {
        configured1 = manager->configure<ThreadPool::ThreadMode::PRIORITY, 1000000, ThreadPool::PrecedenceLevel::HIGH>();
    });

    auto thread2 = std::thread([&]() {
        configured2 = manager->configure<ThreadPool::ThreadMode::STANDARD, 0, ThreadPool::PrecedenceLevel::LOW>();
    });

    thread1.join();
    thread2.join();

    // Ensure only one configuration succeeded
    EXPECT_TRUE(configured1 || configured2);
    EXPECT_TRUE(manager->initialized());
}

// TEST_F(ThreadPoolManagerTest, InvalidTickConfiguration) {
//     // Attempt to configure with an invalid tick value (negative value simulation)
//     EXPECT_THROW((manager->configure<ThreadPool::ThreadMode::STANDARD, -1, ThreadPool::PrecedenceLevel::LOW>()), std::logic_error);
// }

TEST_F(ThreadPoolManagerTest, ThreadPoolReuse) {
    // Configure the thread pool and retrieve it
    EXPECT_TRUE((manager->configure<ThreadPool::ThreadMode::STANDARD, 0, ThreadPool::PrecedenceLevel::MEDIUM>()));
    ThreadPool::ThreadPool<>& pool1 = manager->get_thread_pool();

    // Attempt to reconfigure and retrieve the pool
    EXPECT_FALSE((manager->configure<ThreadPool::ThreadMode::PRIORITY, 1000000, ThreadPool::PrecedenceLevel::LOW>()));
    ThreadPool::ThreadPool<>& pool2 = manager->get_thread_pool();

    // Ensure the same pool instance is reused
    EXPECT_EQ(&pool1, &pool2);
}
