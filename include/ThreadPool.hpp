#pragma once

//--------------------------------------------------------------
// Standard library
//--------------------------------------------------------------
#include <iostream>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <tuple>
#include <future>
#include <variant>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "PriorityQueue.hpp"
#include "ThreadTask.hpp"
//--------------------------------------------------------------
/** @namespace ThreadPool
 * @brief A namespace containing the ThreadPool class.
 */
namespace ThreadPool{
    //--------------------------------------------------------------
    /**
     * @class ThreadPool
     * @brief A thread pool implementation that manages worker threads.
     *
     * The ThreadPool class handles the creation, management, and destruction of threads. 
     * It also provides functionality to queue tasks for execution by the worker threads.
     */
    class ThreadPool {
        //-------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            /**
             * @class TaskBuilder
             * @brief Utility class to construct and manage tasks for the thread pool.
             *
             * This class provides a fluent interface for creating, configuring, and submitting tasks to the ThreadPool.
             * @tparam F The function type for the task.
             * @tparam Args The argument types for the task function.
             */
            template <typename F, typename... Args>
            class TaskBuilder {
                //--------------------------------------------------------------
                private:
                    using ReturnType = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;
                    //--------------------------
                    // Dummy struct for void-returning functions
                    struct VoidType {};
                    //--------------------------------------------------------------
                public:
                    //--------------------------------------------------------------
                    /**
                     * @brief Constructs a TaskBuilder object.
                     *
                     * This constructor creates a TaskBuilder instance for enqueuing tasks into a ThreadPool.
                     * If the provided task has a return type, its future is made available for retrieval.
                     * Optionally, the task can be submitted automatically to the ThreadPool upon creation.
                     *
                     * @tparam F Function type for the task.
                     * @tparam Args Argument types for the task function.
                     *
                     * @param threadPool Reference to the ThreadPool where tasks will be submitted.
                     * @param auto_submit If set to true, the task will be submitted to the thread pool upon creation.
                     * @param f Task function.
                     * @param args Arguments to be passed to the task function.
                     *
                     * @note If the return type of the task function is void, the future associated with the task will be inaccessible.
                     *
                     * @example
                     * ```cpp
                     * ThreadPool::ThreadPool pool;
                     * 
                     * // Queue a task without auto submission.
                     * auto taskBuilder1 = TaskBuilder(pool, false, [](int x) { return x*x; }, 5);
                     * taskBuilder1.submit();
                     * 
                     * // Queue a task with auto submission.
                     * auto taskBuilder2 = TaskBuilder(pool, true, [](int x) { return x*x; }, 10);
                     * 
                     * // Retrieve result.
                     * int result = taskBuilder2.get_future().get();
                     * ```
                     */
                    TaskBuilder(ThreadPool& threadPool, bool auto_submit, F&& f, Args&&... args)
                        :   m_threadPool(threadPool),
                            m_priority(0),
                            m_retries(0),
                            m_submitted(false),
                            m_task(createTask(std::forward<F>(f), std::forward<Args>(args)...)) {
                        //--------------------------
                        if constexpr (!std::is_void_v<ReturnType>) {
                            m_future.emplace(m_task.get_future());
                        }// end if constexpr (!std::is_void_v<ReturnType>)
                        //--------------------------
                        if (auto_submit) {
                            submit();
                        }// end if (auto_submit)
                        //--------------------------
                    }// end TaskBuilder(ThreadPool& threadPool, bool auto_submit, F&& f, Args&&... args)
                    //--------------------------
                    /**
                     * @brief Destructor for the TaskBuilder class.
                     *
                     * Ensures that the task is submitted to the ThreadPool upon destruction if it hasn't been submitted yet.
                     * This is especially useful to guarantee that all tasks are enqueued when a TaskBuilder object goes out of scope.
                     *
                     * @note The destructor will not throw exceptions even if the submission fails, as exceptions should not be thrown from destructors.
                     *
                     * @example
                     * ```cpp
                     * {
                     *     ThreadPool::ThreadPool pool;
                     *     
                     *     // Creating a TaskBuilder instance without auto submission.
                     *     TaskBuilder(pool, false, [](int x) { return x*x; }, 5);
                     *     // No explicit submission is called.
                     *     
                     * }  // As the scope ends, TaskBuilder's destructor ensures the task is submitted to the pool.
                     * ```
                     */
                    inline  ~TaskBuilder(void){
                        submit();
                    }// end ~TaskBuilder(void)
                    //--------------------------
                    /**
                     * @brief Sets the priority for the task encapsulated by this TaskBuilder instance.
                     * 
                     * This method allows the user to specify a priority for the task. Tasks with higher priority values
                     * may be processed before tasks with lower priority, depending on the ThreadPool's implementation and configuration.
                     *
                     * @param p The priority value to set for the task. Expected to be a non-negative integer, with higher values
                     * indicating higher priority.
                     * @return A reference to this TaskBuilder instance, to allow for method chaining.
                     *
                     * @note It is recommended to set priority before submitting the task for execution.
                     *
                     * @example
                     * ```cpp
                     * ThreadPool::ThreadPool pool;
                     * 
                     * // Create and enqueue a task with a specific priority.
                     * pool.queue(true, [](int x) { return x*x; }, 5)
                     *     .set_priority(10);  // Set a priority of 10.
                     * ```
                     */
                    inline TaskBuilder& set_priority(const uint8_t& priority) {
                        m_priority = priority;
                        return *this;
                    }// end TaskBuilder& set_priority(const uint8_t& p)
                    //--------------------------
                    /**
                     * @brief Sets the retry count for the task encapsulated by this TaskBuilder instance.
                     * 
                     * This method allows the user to specify how many times the ThreadPool should attempt 
                     * to retry the task in case of failures or errors during its execution.
                     *
                     * @param r The number of retries to set for the task. Expected to be a non-negative integer.
                     * @return A reference to this TaskBuilder instance, to allow for method chaining.
                     *
                     * @note If a task fails during its execution, the ThreadPool may attempt to rerun the task 
                     * up to the specified number of retries. After the set number of retries has been exhausted, 
                     * the task will not be retried further, and it may be reported as a failed task.
                     *
                     * @example
                     * ```cpp
                     * ThreadPool::ThreadPool pool;
                     * 
                     * // Create and enqueue a task with a specified retry count.
                     * pool.queue(true, [](int x) { return x/x; }, 0)
                     *     .set_retries(3);  // Set the task to retry up to 3 times upon failure.
                     * ```
                     */
                    inline TaskBuilder& set_retries(const uint8_t& retries) {
                        m_retries = retries;
                        return *this;
                    }// end TaskBuilder& set_retries(const uint8_t& r)
                    //--------------------------
                    /**
                     * @brief Submits the task encapsulated by this TaskBuilder to the associated ThreadPool for execution.
                     * 
                     * If the task has already been submitted (or automatically submitted upon TaskBuilder creation),
                     * this method does nothing. Otherwise, it places the task into the ThreadPool's queue with the 
                     * specified priority and retry count. Once submitted, the task will be picked up by one of 
                     * the worker threads in the ThreadPool and executed.
                     *
                     * @note This method is idempotent; calling it multiple times will not result in the task being 
                     * queued multiple times. 
                     * 
                     * @warning Ensure that the ThreadPool to which this task is being submitted is not destroyed 
                     * before the task has a chance to execute, otherwise the behavior is undefined.
                     * 
                     * @example
                     * ```cpp
                     * ThreadPool::ThreadPool pool;
                     * 
                     * // Create a task without auto-submitting it.
                     * auto task = pool.queue(false, [](int x) { return x*x; }, 5)
                     *                .set_priority(3)
                     *                .set_retries(2);
                     * 
                     * // Manually submit the task at a later point.
                     * task.submit();
                     * ```
                     */
                    void submit(void) {
                        if (!m_submitted) {
                            m_threadPool.emplace_task(std::move(m_task), m_priority, m_retries);
                            m_submitted = true;
                        }// end if (!m_submitted)
                    }// end void submit(void) 
                    //--------------------------
                    /**
                     * @brief This version of the `get_future` method is explicitly deleted for tasks with a void return type.
                     * 
                     * Tasks with a void return type do not have an associated future because there's no result to retrieve.
                     * Attempting to call this method for such tasks will result in a compilation error.
                     * 
                     * @tparam T Type of the return value of the task. Defaults to the ReturnType of the task function.
                     * 
                     * @throws This method is deleted and will cause a compilation error if called.
                     */
                    template <typename T = ReturnType>
                    std::enable_if_t<std::is_void_v<T>, void> get_future(void) = delete;
                    //--------------------------
                    /**
                     * @brief Retrieves the future associated with this task, allowing the caller to get the result once it's available.
                     * 
                     * This method provides access to the future associated with the task encapsulated by this TaskBuilder. This future 
                     * can be used to retrieve the result of the task once it has been executed by a thread in the ThreadPool.
                     *
                     * @tparam T Type of the return value of the task. Defaults to the ReturnType of the task function.
                     * 
                     * @return A future associated with the result of the task. The future can be used to retrieve the result.
                     *
                     * @throws std::future_error with an error code of `std::future_errc::no_state` if the future is not available, 
                     *         typically because the task has a void return type or the future has already been retrieved.
                     * 
                     * @example
                     * ```cpp
                     * ThreadPool::ThreadPool pool;
                     * 
                     * // Create and auto-submit a task.
                     * auto taskBuilder = pool.queue(true, [](int x) { return x*x; }, 5);
                     * 
                     * // Retrieve the future and get the result.
                     * std::future<int> resultFuture = taskBuilder.get_future();
                     * int result = resultFuture.get();
                     * std::cout << "Result: " << result << std::endl; // Prints "Result: 25"
                     * ```
                     */
                    template <typename T = ReturnType>
                    std::enable_if_t<!std::is_void_v<T>, std::future<T>> get_future(void) {
                        if(m_future) {
                            return std::move(*m_future);
                        } // end if(m_future)
                        //--------------------------
                        throw std::future_error(std::future_errc::no_state);
                        //--------------------------
                    }// end std::enable_if_t<!std::is_void_v<T>, std::future<T>> get_future(void)
                    //--------------------------
                    /**
                     * @brief This version of the `get` method is explicitly deleted for tasks with a void return type.
                     * 
                     * Tasks with a void return type do not produce a result. Thus, calling `get` for such tasks is not meaningful.
                     * Attempting to call this method for tasks with a void return type will result in a compilation error.
                     * 
                     * @tparam T Type of the return value of the task. Defaults to the ReturnType of the task function.
                     * 
                     * @throws This method is deleted and will cause a compilation error if called.
                     */
                    template <typename T = ReturnType>
                    std::enable_if_t<std::is_void_v<T>> get(void) = delete;
                    //--------------------------
                    /**
                     * @brief Retrieves the result of the task once it has been computed by the ThreadPool.
                     * 
                     * This method allows the caller to obtain the result of the task encapsulated by this TaskBuilder, 
                     * after it has been executed by a thread in the ThreadPool. This method effectively calls the associated
                     * future's `get` method to obtain the result.
                     * 
                     * Note: Once the result has been retrieved using this method, it cannot be obtained again using the same TaskBuilder.
                     *
                     * @tparam T Type of the return value of the task. Defaults to the ReturnType of the task function.
                     * 
                     * @return The computed result of the task.
                     * 
                     * @throws Any exception that might be thrown by the task itself will be propagated. Additionally, if the result 
                     *         has already been retrieved or if no result is available, a `std::future_error` will be thrown.
                     * 
                     * @example
                     * ```cpp
                     * ThreadPool::ThreadPool pool;
                     * 
                     * // Create and auto-submit a task.
                     * auto taskBuilder = pool.queue(true, [](int x) { return x*x; }, 5);
                     * 
                     * // Retrieve the result directly.
                     * int result = taskBuilder.get();
                     * std::cout << "Result: " << result << std::endl; // Prints "Result: 25"
                     * ```
                     */
                    template <typename T = ReturnType>
                    std::enable_if_t<!std::is_void_v<T>, T> get(void) {
                        auto res = m_future->get();
                        m_future.reset();  // Prevent future gets
                        return res;
                    }// end std::enable_if_t<!std::is_void_v<T>, T> get(void)
                    //--------------------------------------------------------------
                protected:
                    //--------------------------------------------------------------
                    /**
                     * @brief Constructs a `std::packaged_task` based on the provided function and its arguments.
                     * 
                     * The `createTask` method is responsible for creating a `std::packaged_task` that encapsulates the 
                     * provided function and its arguments. This packaged task, when executed, will invoke the function with 
                     * the given arguments.
                     * 
                     * If the provided function's return type is void, the returned `std::packaged_task` is specialized to 
                     * return an internal `VoidType` instead of void.
                     * 
                     * @tparam Func The type of the function or callable object.
                     * @tparam CArgs The types of the captured arguments to be passed to the function.
                     * 
                     * @param func The function or callable object to be wrapped.
                     * @param capturedArgs Variadic arguments that are to be passed to the function when the task is executed.
                     * 
                     * @return A `std::packaged_task` that encapsulates the provided function and its arguments.
                     * 
                     * @example
                     * ```cpp
                     * // Simple function that adds two numbers
                     * int add(int x, int y) {
                     *     return x + y;
                     * }
                     * 
                     * ThreadPool::TaskBuilder taskBuilder(/*...some initialization params...*);
                     * auto task = taskBuilder.createTask(add, 5, 3);
                     * 
                     * task();  // Invokes the encapsulated function, i.e., add(5, 3)
                     * ```
                     */
                    template <typename Func, typename... CArgs>
                    auto createTask(Func&& func, CArgs&&... capturedArgs) {
                        //--------------------------
                        if constexpr (!std::is_void_v<ReturnType>) {
                            //--------------------------
                            return std::packaged_task<ReturnType()>(
                                [f = std::forward<Func>(func), ...args = std::forward<CArgs>(capturedArgs)]() mutable {
                                    return std::invoke(f, std::forward<CArgs>(args)...);
                                }
                            );
                            //--------------------------
                        } else {
                            //--------------------------
                            return std::packaged_task<VoidType()>(
                                [f = std::forward<Func>(func), ...args = std::forward<CArgs>(capturedArgs)]() mutable {
                                    std::invoke(f, std::forward<CArgs>(args)...);
                                    return VoidType{};
                                }
                            );
                            //--------------------------
                        }// end else if constexpr (!std::is_void_v<ReturnType>)
                    }// end auto createTask(Func&& func, CArgs&&... capturedArgs)
                    //--------------------------------------------------------------
                private:
                    //--------------------------------------------------------------
                    ThreadPool& m_threadPool;
                    uint8_t m_priority, m_retries;
                    bool m_submitted;
                    std::packaged_task<std::conditional_t<std::is_void_v<ReturnType>, VoidType, ReturnType>()> m_task;
                    std::optional<std::future<ReturnType>> m_future;
                //--------------------------------------------------------------
            };// end class TaskBuilder
            //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            ThreadPool(void) = delete;
            //--------------------------
            /**
             * @brief Constructs a ThreadPool with the specified number of worker threads.
             * 
             * The constructor initializes a ThreadPool with a specified number of worker threads.
             * The actual number of worker threads will be clamped between the system's lower
             * threshold and either the upper threshold or the number of hardware threads available,
             * whichever is smaller.
             *
             * Internally, the ThreadPool will keep track of tasks that have failed, been retried, 
             * or completed successfully.
             * 
             * @param numThreads Desired number of worker threads. This number will be clamped 
             * between the system's predefined lower and upper thresholds.
             *
             * @example
             * 
             * @code
             * 
             * #include "ThreadPool.hpp"
             * 
             * int main() {
             *     // Create a ThreadPool with 4 worker threads
             *     ThreadPool::ThreadPool pool(4);
             *     
             *     // Now, you can queue tasks to the pool...
             *     
             *     return 0;
             * }
             * 
             * @endcode
             *
             * @note The number of threads provided should ideally be less than or equal to the number of hardware threads available. 
             * In cases where the provided number is greater than the number of hardware threads or less than the lower threshold,
             * it will be adjusted accordingly.
             */
            explicit ThreadPool(const size_t& numThreads = static_cast<size_t>(std::thread::hardware_concurrency()));
            //--------------------------
            ThreadPool(const ThreadPool&)            = delete;
            ThreadPool& operator=(const ThreadPool&) = delete;
            //----------------------------
            ThreadPool(ThreadPool&&)                 = delete;
            ThreadPool& operator=(ThreadPool&&)      = delete;
            //--------------------------
            ~ThreadPool(void);
            //--------------------------
             /**
             * @brief Retrieves the number of active worker threads in the thread pool.
             * 
             * This function provides a safe way to query the number of active worker threads
             * currently managed by the ThreadPool instance. It internally acquires a lock to ensure 
             * thread safety while accessing shared resources.
             *
             * @return size_t The number of active worker threads.
             *
             * @note This function is thread-safe.
             *
             * @section Example Usage
             * 
             * @code
             * #include "ThreadPool.hpp"
             * 
             * int main() {
             *     ThreadPool::ThreadPool pool(4); // create a thread pool with 4 workers
             *     
             *     // ... submit some tasks to the pool ...
             *     
             *     std::cout << "Active workers: " << pool.threads_size() << std::endl;
             *     
             *     return 0;
             * }
             * @endcode
             */
            size_t threads_size(void) const;
            //--------------------------
            /**
             * @brief Retrieves the current number of active tasks in the ThreadPool.
             * 
             * The `active_tasks_size` function returns the number of tasks that are currently
             * in the queue waiting to be executed by the worker threads of the ThreadPool.
             * 
             * @return size_t The number of tasks currently in the ThreadPool's queue.
             *
             * @example
             * 
             * @code
             * 
             * #include "ThreadPool.hpp"
             * 
             * int main() {
             *     ThreadPool::ThreadPool pool(4);
             *     
             *     // Queue some tasks to the pool...
             *     
             *     // Print the number of active tasks in the pool
             *     std::cout << "Number of active tasks: " << pool.queued_size() << std::endl;
             *     
             *     return 0;
             * }
             * 
             * @endcode
             *
             * @note This function only returns the number of tasks that are in the queue and does
             * not account for tasks currently being executed by the worker threads.
             */
            size_t queued_size(void) const;
            //--------------------------
            std::tuple<size_t, size_t, size_t> status(void);
            //--------------------------
            void status_disply(void);
            //--------------------------
            /**
             * @brief Queues a task for execution in the ThreadPool.
             * 
             * Creates a TaskBuilder instance for the provided task function and its arguments.
             * The task is then queued for execution in the ThreadPool.
             * 
             * @tparam F Function type of the task.
             * @tparam Args Parameter pack for the function arguments.
             * 
             * @param auto_submit Automatically submits the task to the thread pool if set to true.
             * @param f Function representing the task.
             * @param args Arguments to be passed to the task function.
             * 
             * @return TaskBuilder An instance of TaskBuilder that allows further configuration 
             *                     and management of the task.
             * 
             * @example
             * ThreadPool pool(4);
             * auto task = pool.queue(true, []() {
             *     // task code here...
             * });
             */
            template <class F, class... Args>
            auto queue(bool auto_submit, F&& f, Args&&... args){
                //--------------------------
                return enqueue<F, Args...>(auto_submit, std::forward<F>(f), std::forward<Args>(args)...);
                //--------------------------
            }// end TaskBuilder queue(F&& f, Args&&... args)
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            template <class F, class... Args>
            auto enqueue(bool auto_submit, F&& f, Args&&... args) {
                //--------------------------
                return TaskBuilder<F, Args...>(*this, auto_submit, std::forward<F>(f), std::forward<Args>(args)...);
                //--------------------------
            }// end TaskBuilder enqueue(F&& f, Args&&... args)            
            //--------------------------------------------------------------
            void create_task(const size_t& numThreads);
            //--------------------------
            void workerFunction(const std::stop_token& stoken);
            //--------------------------
            void adjustWorkers(void);
            //--------------------------
            void adjustmentThreadFunction(const std::stop_token& stoken);
            //--------------------------
            void stop(void);
            //--------------------------
            void push_task(ThreadTask&& task);
            //--------------------------
            template<typename... Args>
            void emplace_task(Args&&... args){
                //--------------------------
                m_tasks.emplace(std::forward<Args>(args)...);
                //--------------------------
                m_taskAvailableCondition.notify_one();
                //--------------------------
            }// end void emplace_task(Args&&... args)
            //--------------------------
            void handleError(ThreadTask&& task, const char* error);
            //--------------------------
            size_t thread_Workers_size(void) const;
            //--------------------------
            size_t active_tasks_size(void) const;
            //--------------------------
            std::tuple<size_t, size_t, size_t> get_status(void);
            //--------------------------
            void status_display_internal(void);
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            std::atomic<size_t> m_failedTasksCount, m_retriedTasksCount, m_completedTasksCount;
            //--------------------------
            const size_t m_upperThreshold;
            //--------------------------
            std::vector<std::jthread> m_workers;
            //--------------------------
            PriorityQueue<ThreadTask> m_tasks;
            //--------------------------
            std::jthread m_adjustmentThread;
            //--------------------------
            mutable std::mutex m_mutex;
            //--------------------------
            std::condition_variable m_taskAvailableCondition, m_allStoppedCondition;
        //--------------------------------------------------------------
    };// end class ThreadPool
    //--------------------------------------------------------------
}//end namespace ThreadPool
//--------------------------------------------------------------