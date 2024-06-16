#pragma once

//--------------------------------------------------------------
// Standard library
//--------------------------------------------------------------
#include <iostream>
#include <condition_variable>
#include <thread>
#include <future>
#include <chrono>
#include <concepts>
#include <unordered_set>
#include <unordered_map>
#include <cmath>
#include <limits>
#include <functional>
#include <optional>
#include <mutex>
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
     * @enum ThreadMode
     * @brief Enum class to specify threading mode for ThreadPool.
     *
     * This enum class is used to define the threading mode for the ThreadPool class. It supports two modes:
     * STANDARD and PRIORITY. STANDARD mode is used for regular operation, while PRIORITY mode allows for
     * priority-based task execution within the ThreadPool.
     *
     * @param ThreadMode::STANDARD
     * STANDARD mode indicates that the ThreadPool will process tasks in a first-come, first-served manner,
     * without considering task priority. This is the default mode.
     *
     * @param ThreadMode::PRIORITY
     * PRIORITY mode indicates that the ThreadPool will process tasks based on their priority, allowing
     * higher priority tasks to be executed before lower priority ones.
     */
    enum class ThreadMode : bool {
        STANDARD = false,
        PRIORITY = true
    }; // end enum class ThreadMode
    //--------------------------------------------------------------
    /**
     * @class ThreadPool
     * @brief A thread pool implementation that manages worker threads.
     *
     * The ThreadPool class handles the creation, management, and destruction of threads. 
     * It also provides functionality to queue tasks for execution by the worker threads.
     */
    template <ThreadMode use_priority_queue = ThreadMode::STANDARD, size_t adoptive_tick = 1000000UL>
    class ThreadPool {
        //-------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            /**
             * @class TaskBuilder
             * @brief Builds and manages the life-cycle of a task to be executed by a ThreadPool with priority queue support.
             * 
             * @details The TaskBuilder class is responsible for creating tasks, setting their properties like priority and retries,
             * and managing their submission to the thread pool. Tasks can either be auto-submitted upon construction or manually
             * submitted later. This class is specifically designed to work when the `use_priority_queue` is enabled.
             *
             * @note This class should not be used directly if `use_priority_queue` is disabled.
             * 
             * @tparam F Type of the callable (like function, lambda, etc.)
             * @tparam Args Variadic types of the arguments to be passed to the callable.
             *
             * @example:
             * @code
             * ThreadPool pool; // ThreadPool must be instantiated with priority queue support enabled.
             * auto task = pool.enqueue(true, [](int a, int b) { return a + b; }, 2, 3);
             * task.set_priority(10).set_retries(3);
             * auto result = task.get();  // Will retrieve the result after task execution.
             * @endcode
             */
            template <typename F, typename... Args> requires (static_cast<bool>(use_priority_queue))
            class TaskBuilder {
                //--------------------------------------------------------------
                // static_assert(!use_priority_queue, "TaskBuilder can only be used with priority queues disable.");
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
                     * @details This constructor creates a TaskBuilder instance for enqueuing tasks into a ThreadPool.
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
                            m_task(create_task(std::forward<F>(f), std::forward<Args>(args)...)) {
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
                     * @details Ensures that the task is submitted to the ThreadPool upon destruction if it hasn't been submitted yet.
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
                     * @details This method allows the user to specify a priority for the task. Tasks with higher priority values
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
                     * @details This method allows the user to specify how many times the ThreadPool should attempt 
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
                     * @details If the task has already been submitted (or automatically submitted upon TaskBuilder creation),
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
                     * @details Tasks with a void return type do not have an associated future because there's no result to retrieve.
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
                     * @details This method provides access to the future associated with the task encapsulated by this TaskBuilder. This future 
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
                     * @details Tasks with a void return type do not produce a result. Thus, calling `get` for such tasks is not meaningful.
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
                     * @details This method allows the caller to obtain the result of the task encapsulated by this TaskBuilder, 
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
                     * @details The `create_task` method is responsible for creating a `std::packaged_task` that encapsulates the 
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
                     * ThreadPool::TaskBuilder taskBuilder(//...some initialization params...);
                     * auto task = taskBuilder.create_task(add, 5, 3);
                     * 
                     * task();  // Invokes the encapsulated function, i.e., add(5, 3)
                     * ```
                     */
                    template <typename Func, typename... CArgs>
                    auto create_task(Func&& func, CArgs&&... capturedArgs) {
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
                    }// end auto create_task(Func&& func, CArgs&&... capturedArgs)
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
             * @details The constructor initializes a ThreadPool with a specified number of worker threads.
             * The actual number of worker threads will be clamped between the system's lower
             * threshold and either the upper threshold or the number of hardware threads available,
             * whichever is smaller.
             *
             * Internally, the ThreadPool will keep track of tasks that have failed, been retried, 
             * or completed successfully.
             * 
             * @param number_threads Desired number of worker threads. This number will be clamped 
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
            explicit ThreadPool(const size_t& number_threads = static_cast<size_t>(std::thread::hardware_concurrency()))
                            :   m_upper_threshold((static_cast<size_t>(std::thread::hardware_concurrency()) > 1) ? 
                                    static_cast<size_t>(std::thread::hardware_concurrency()) : m_lower_threshold),
                                m_adoptive_thread(assign_adoptive_thread()){
                //--------------------------
                auto _threads_number = std::clamp(number_threads, m_lower_threshold, m_upper_threshold);
                //--------------------------
                if constexpr (adoptive_tick > 0UL){
                    m_idle_threads.emplace();
                    m_idle_threads->reserve(_threads_number);
                }//end if constexpr (adoptive_tick > 0UL)
                //--------------------------
                create_task(_threads_number);
                //--------------------------
                if constexpr (static_cast<bool>(use_priority_queue)){
                    m_tasks.reserve(number_threads);
                }//end if constexpr (static_cast<bool>(use_priority_queue))
                //--------------------------
            }// end ThreadPool(const size_t& number_threads = static_cast<size_t>(std::thread::hardware_concurrency()))
            //--------------------------
            ThreadPool(const ThreadPool&)            = delete;
            ThreadPool& operator=(const ThreadPool&) = delete;
            //----------------------------
            ThreadPool(ThreadPool&&)                 = delete;
            ThreadPool& operator=(ThreadPool&&)      = delete;
            //--------------------------
            ~ThreadPool(void){
                //--------------------------
                stop();
                //--------------------------
            }// end ~ThreadPool(void) 
            //--------------------------
             /**
             * @brief Retrieves the number of active worker threads in the thread pool.
             * 
             * @details This function provides a safe way to query the number of active worker threads
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
            size_t threads_size(void) const{
                //--------------------------
                return thread_Workers_size();
                //--------------------------
            }// end size_t ThreadPool::ThreadPool::threads_size() const
            //--------------------------
            /**
             * @brief Retrieves the current number of active tasks in the ThreadPool.
             * 
             * @details The `active_tasks_size` function returns the number of tasks that are currently
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
            size_t queued_size(void) const{
                //--------------------------
                return active_tasks_size();
                //--------------------------
            }// end size_t ThreadPool::ThreadPool::queued_size() const
            //--------------------------
            /**
             * @brief Enqueues a task with priority and retry settings by returning a TaskBuilder object.
             *
             * @details This method creates a TaskBuilder instance that allows the caller to set additional
             * properties for the task such as priority and number of retries. This overload of `enqueue` is 
             * specifically enabled when `use_priority_queue` is true, allowing tasks to be queued in a priority-based fashion.
             *
             * @note The task is not submitted to the thread pool until the TaskBuilder's `submit` method is called, or it is 
             * auto-submitted based on the `auto_submit` parameter.
             *
             * @param auto_submit Automatically submits the task for execution if set to true.
             * 
             * @tparam F Type of the callable to be executed.
             * @tparam Args Variadic template for the arguments list of the callable.
             * 
             * @return TaskBuilder<F, Args...> A builder object for setting task properties and submitting the task.
             * 
             * @example:
             * @code
             * ThreadPool pool; // ThreadPool must be instantiated with priority queue support enabled.
             * auto task = pool.queue(true, [](int a, int b) { return a + b; }, 2, 3);
             * task.set_priority(5).set_retries(2); // Optional: set priority and retries.
             * auto result = task.get(); // Will wait for the task to complete and retrieve the result.
             * @endcode
             * @code
             * ThreadPool pool; // ThreadPool must be instantiated with priority queue support enabled.
             * auto task = pool.queue(true, [](int a, int b) { return a + b; }, 2, 3).set_priority(5).set_retries(2); // Optional: set priority and retries.
             * auto result = task.get(); // Will wait for the task to complete and retrieve the result.
             * @endcode
             */
            template <class F, class... Args>
            std::enable_if_t<static_cast<bool>(use_priority_queue), TaskBuilder<F, Args...>> queue(bool auto_submit, F&& f, Args&&... args){
                //--------------------------
                return TaskBuilder<F, Args...>(*this, auto_submit, std::forward<F>(f), std::forward<Args>(args)...);
                //--------------------------
            }// end TaskBuilder queue(F&& f, Args&&... args)// end TaskBuilder queue(F&& f, Args&&... args)
            //--------------------------
            /**
             * @brief Queue a new task in the thread pool.
             *
             * @details This version of the `queue` function template is enabled only when
             * the `use_priority_queue` is false and the return type of the callable is not void.
             * It creates a task from the provided callable object (like a function, lambda expression,
             * bind expression, or another function object) and its arguments, then enqueues this task
             * for execution in the thread pool. The function returns a future representing the
             * asynchronous execution of the task.
             *
             *
             * @tparam F Type of the callable to be executed.
             * @tparam Args Variadic template for the arguments list of the callable.
             * @return std::future<std::invoke_result_t<F, Args...>> A future object representing the asynchronous execution of the callable.
             * 
             * @example
             * @code
             * ThreadPool pool;
             * auto future = pool.enqueue([](int a, int b) { return a + b; }, 2, 3);
             * auto result = future.get();  // result will hold the sum 5 after the task completes
             * @endcode
             */
            template <class F, class... Args>
            std::enable_if_t<!static_cast<bool>(use_priority_queue) and !std::is_void_v<std::invoke_result_t<F, Args...>>, std::future<std::invoke_result_t<F, Args...>>>
            queue(F&& f, Args&&... args){
                //--------------------------
                return enqueue(std::forward<F>(f), std::forward<Args>(args)...);
                //--------------------------
            }// end TaskBuilder queue(F&& f, Args&&... args)
            //--------------------------
            /**
             * @brief Queue a new task in the thread pool.
             *
             * @details This version of the `queue` function template is enabled only when
             * the `use_priority_queue` is false and the return type of the callable is void.
             * It creates a task from the provided callable object and its arguments, then enqueues this
             * task for execution in the thread pool. Unlike its counterpart which handles non-void callables,
             * this function does not return any future, as there is no return value from the callable.
             *
             * @tparam F Type of the callable to be executed.
             * @tparam Args Variadic template for the arguments list of the callable.
             * 
             * @example:
             * @code
             * ThreadPool pool;
             * pool.queue([]() { std::cout << "Task is being executed." << std::endl; });
             * // The task is enqueued and will output to the console when executed.
             * @endcode
             */
            template <class F, class... Args>
            std::enable_if_t<!static_cast<bool>(use_priority_queue) && std::is_void_v<std::invoke_result_t<F, Args...>>, void> queue(F&& f, Args&&... args){
                //--------------------------
                enqueue(std::forward<F>(f), std::forward<Args>(args)...);
                //--------------------------
            }// end TaskBuilder queue(F&& f, Args&&... args)
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            template <class F, class... Args>
            std::enable_if_t<static_cast<bool>(use_priority_queue), TaskBuilder<F, Args...>> enqueue(bool auto_submit, F&& f, Args&&... args) {
                //--------------------------
                return TaskBuilder<F, Args...>(*this, auto_submit, std::forward<F>(f), std::forward<Args>(args)...);
                //--------------------------
            }// end TaskBuilder enqueue(F&& f, Args&&... args)            
            //--------------------------------------------------------------
            template <class F, class... Args>
            std::enable_if_t<!static_cast<bool>(use_priority_queue) && !std::is_void_v<std::invoke_result_t<F, Args...>>, std::future<std::invoke_result_t<F, Args...>>>
            enqueue(F&& f, Args&&... args) {
                //--------------------------
                using ReturnType = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;
                //--------------------------
                auto taskFn = [f = std::forward<F>(f), ...capturedArgs = std::forward<Args>(args)]() mutable -> ReturnType {
                    return std::invoke(f, std::forward<Args>(capturedArgs)...);
                };
                //--------------------------
                auto packagedTaskPtr = std::make_shared<std::packaged_task<ReturnType()>>(std::move(taskFn));
                //--------------------------
                std::future<ReturnType> future = packagedTaskPtr->get_future();
                //--------------------------
                {// adding task
                    std::lock_guard lock(m_mutex);
                    m_tasks.emplace_back([pt = packagedTaskPtr]() { (*pt)(); });
                }// end adding task
                //--------------------------
                m_task_available_condition.notify_one();
                //--------------------------
                return future;
                //--------------------------
            }// end TaskBuilder enqueue(F&& f, Args&&... args)            
            //--------------------------------------------------------------
            template <class F, class... Args>
            std::enable_if_t<!static_cast<bool>(use_priority_queue) && std::is_void_v<std::invoke_result_t<F, Args...>>, void>
            enqueue(F&& f, Args&&... args) {
                //--------------------------
                using ReturnType = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;
                //--------------------------
                auto taskFn = [f = std::forward<F>(f), ...capturedArgs = std::forward<Args>(args)]() mutable -> ReturnType {
                    return std::invoke(f, std::forward<Args>(capturedArgs)...);
                };
                //--------------------------
                auto packagedTaskPtr = std::make_shared<std::packaged_task<ReturnType()>>(std::move(taskFn));
                //--------------------------
                {// adding task
                    std::lock_guard lock(m_mutex);
                    m_tasks.emplace_back([pt = packagedTaskPtr]() { (*pt)(); });
                }// end adding task
                //--------------------------
                m_task_available_condition.notify_one();
                //--------------------------
            }// end enqueue(F&& f, Args&&... args)
            //--------------------------------------------------------------
            template <size_t U = adoptive_tick>
            std::enable_if_t<(U > 0UL), void> create_task(const size_t& number_threads){
                //--------------------------
                static thread_local size_t id = 0UL;
                //--------------------------
                m_workers.reserve(m_workers.size() + number_threads);
                //--------------------------
                for (size_t i = 0; i < number_threads; ++i) {
                    //--------------------------
                    auto safe_id = safe_increment(id);
                    //--------------------------
                    if(!safe_id.has_value()){
                        break;
                    }// end if(!safe_id.has_value())
                    //--------------------------
                    id = safe_id.value();
                    //--------------------------
                    m_workers.emplace(id, [this, local_id = id](std::stop_token stoken) {
                        this->worker_function(stoken, local_id);
                    });
                    //--------------------------
                }// end  for (size_t i = 0; i < number_threads; ++i)
                //--------------------------
            }//end void ThreadPool::ThreadPool::create_task(const size_t& number_threads)
            //--------------------------
            template <size_t U = adoptive_tick>
            std::enable_if_t<!U , void> create_task(const size_t& number_threads){
                //--------------------------
                m_workers.reserve(m_workers.size() + number_threads);
                //--------------------------
                for (size_t i = 0; i < number_threads; ++i) {
                    //--------------------------
                    m_workers.emplace_back([this](std::stop_token stoken) {
                        this->worker_function(stoken);
                    });
                    //--------------------------
                }// end  for (size_t i = 0; i < number_threads; ++i)
                //--------------------------
            }//end void ThreadPool::ThreadPool::create_task(const size_t& number_threads)
            //--------------------------
            void worker_function(const std::stop_token& stoken, const std::optional<size_t> id = std::nullopt){
                //--------------------------
                while (!stoken.stop_requested()) {
                    //--------------------------
                    using TaskType = std::conditional_t<static_cast<bool>(use_priority_queue), ThreadTask, std::function<void()>>;
                    TaskType task;
                    //--------------------------
                    {// being Append tasks 
                        //--------------------------
                        std::unique_lock lock(m_mutex);
                        //--------------------------
                        if constexpr (adoptive_tick > 0UL){
                            m_idle_threads->insert(id.value());
                        }// end if constexpr (adoptive_tick > 0UL)
                        //--------------------------
                        m_task_available_condition.wait(lock, [this, &stoken] {return stoken.stop_requested() or !m_tasks.empty();});
                        //--------------------------
                        if constexpr (adoptive_tick > 0UL){
                            m_idle_threads->erase(id.value());
                        }// end if constexpr (adoptive_tick > 0UL)
                        //--------------------------
                        if (stoken.stop_requested() or m_tasks.empty()) {
                            //--------------------------
                            m_all_stopped_condition.notify_one();
                            //--------------------------
                            return;
                            //--------------------------
                        }// end if (stoken.stop_requested() and m_tasks.empty())
                        //--------------------------
                        if constexpr (static_cast<bool>(use_priority_queue)){
                            task = std::move(m_tasks.pop_top().value());
                        } else{
                            task = std::move(m_tasks.front());
                            m_tasks.pop_front();
                        }// end if constexpr (static_cast<bool>(use_priority_queue))
                        //--------------------------
                    }// end Append tasks
                    //--------------------------
                    try {
                        //--------------------------
                        if constexpr (static_cast<bool>(use_priority_queue)){
                            static_cast<void>(task.try_execute());
                        } else{
                            task();
                        }// end if constexpr (static_cast<bool>(use_priority_queue))
                        //--------------------------
                    } // end try
                    catch (const std::exception& e) {
                        //--------------------------
                        if constexpr (static_cast<bool>(use_priority_queue)){
                            handle_error(std::move(task), e.what());
                        } else{
                            handle_error(e.what());
                        }// end if constexpr (static_cast<bool>(use_priority_queue))
                        //--------------------------
                    } // end catch (const std::exception& e)
                    catch (...) {
                        //--------------------------
                        if constexpr (static_cast<bool>(use_priority_queue)){
                            handle_error(std::move(task), "Unknown error");
                        } else{
                            handle_error("Unknown error");
                        }// end if constexpr (static_cast<bool>(use_priority_queue))
                        //--------------------------
                    }// end catch (...)
                    //--------------------------
                }// end while (!stoken.stop_requested())
                //--------------------------
            }// end void worker_function(void)
            //--------------------------
            template <size_t U = adoptive_tick>
            std::enable_if_t<(U > 0UL), void> adjust_workers(void){
                //--------------------------
                static const size_t threshold_ = static_cast<size_t>(std::ceil(m_upper_threshold*0.2));
                //--------------------------
                const size_t task_count = active_tasks_size(), worker_count = thread_Workers_size();
                //--------------------------
                {
                    //--------------------------
                    std::unique_lock lock(m_mutex);
                    //--------------------------
                    if (worker_count > task_count and !m_idle_threads->empty() and worker_count > threshold_) {
                        //--------------------------
                        size_t thread_id_ = *m_idle_threads->begin();
                        //--------------------------
                        m_workers.at(thread_id_).request_stop();
                        m_task_available_condition.notify_all(); // Notify all threads to check for stop request
                        //--------------------------
                        if (m_workers.at(thread_id_).get_stop_token().stop_requested()) {
                            //--------------------------
                            m_idle_threads->erase(thread_id_);
                            //--------------------------
                            lock.unlock(); // Release the lock before joining the thread
                            //--------------------------
                            m_workers.at(thread_id_).join();
                            m_workers.erase(thread_id_);
                            //--------------------------
                            lock.lock(); // Reacquire the lock after joining the thread
                            //--------------------------
                        }//end if (m_workers.at(thread_id_).get_stop_token().stop_requested())
                        //--------------------------
                    }// end if (worker_count > task_count and !m_idle_threads.empty() and worker_count > threshold_)
                    //--------------------------
                    if (task_count > worker_count and worker_count < m_upper_threshold) {
                        create_task(std::min(task_count - worker_count, m_upper_threshold - worker_count));
                    }// if (task_count > worker_count and worker_count < m_upper_threshold)
                    //--------------------------
                }
                //--------------------------
                m_all_stopped_condition.notify_one();
                //--------------------------
            }// end void adjust_workers(void)
            //--------------------------
            template <size_t U = adoptive_tick>
            std::enable_if_t<!U, void> adjust_workers(void) = delete;
            //--------------------------
            template <size_t U = adoptive_tick>
            std::enable_if_t<U > 0UL, void> adjustment_thread_function(const std::stop_token& stoken){
                //--------------------------
                while (!stoken.stop_requested()) {
                    //--------------------------
                    std::this_thread::sleep_for(CHECK_INTERVAL);
                    //--------------------------
                    adjust_workers();
                    //--------------------------
                }// end while (!stoken.stop_requested())
                //--------------------------
            }// end void adjustment_thread_function(const std::stop_token& stoken)
            //--------------------------
            template <size_t U = adoptive_tick>
            std::enable_if_t<!U, void> adjustment_thread_function(const std::stop_token& stoken) = delete;
            //--------------------------
            void stop(void){
                //--------------------------
                {
                    //--------------------------
                    std::unique_lock lock(m_mutex);
                    //--------------------------
                    m_all_stopped_condition.wait(lock, [this] { return m_tasks.empty(); }); 
                    //--------------------------
                    if constexpr (adoptive_tick > 0UL) {
                        m_adoptive_thread->request_stop();
                    } // end if constexpr (adoptive_tick > 0UL)
                    //--------------------------
                    if(!m_workers.empty()){ 
                        if constexpr (adoptive_tick > 0UL) {
                            for (auto &[id, worker] : m_workers) {
                                worker.request_stop(); // request each worker to stop
                            }// end for (auto &worker : m_workers)
                        } else {
                            for (auto &worker : m_workers) {
                                worker.request_stop(); // request each worker to stop
                            }// end for (auto &worker : m_workers)
                        }// end if constexpr (adoptive_tick > 0UL)
                    }// end if(!m_workers.empty())
                }
                //--------------------------
                m_task_available_condition.notify_all();
                //--------------------------
            }//end void stop(void)
            //--------------------------
            template <bool U = static_cast<bool>(use_priority_queue), typename = std::enable_if_t<U>>
            void push_task(ThreadTask&& task){
                //--------------------------
                m_tasks.push(std::move(task));
                //--------------------------
                m_task_available_condition.notify_one();
                //--------------------------
            }// end void push_task(ThreadTask&& task)
            //--------------------------
            template<typename... Args, bool U = static_cast<bool>(use_priority_queue), typename = std::enable_if_t<U>>
            void emplace_task(Args&&... args){
                //--------------------------
                m_tasks.emplace(std::forward<Args>(args)...);
                //--------------------------
                m_task_available_condition.notify_one();
                //--------------------------
            }// end void emplace_task(Args&&... args)
            //--------------------------
            // Method for the case when priority queue is used
            template <bool U = static_cast<bool>(use_priority_queue), typename std::enable_if_t<U, int> = 0>
            void handle_error(ThreadTask&& task, const char* error){
                if (task.get_retries() > 0) {
                    //--------------------------
                    std::scoped_lock lock(m_mutex);
                    //--------------------------;
                    task.decrease_retries();
                    m_tasks.push(std::move(task));
                    //--------------------------
                } else {
                    //--------------------------
                    std::cerr << "Error in task after multiple retries: " << error << std::endl;
                    //--------------------------
                }// end if (task.get_retries() > 0)
                //--------------------------
            }// end void handle_error(ThreadTask&& task, const char* error)
            //--------------------------
            // Method for the case when priority queue is NOT used
            template <bool U = static_cast<bool>(use_priority_queue), typename std::enable_if_t<!U, int> = 0>
            void handle_error(const char* error){
                //--------------------------
                std::cerr << "Error in task: " << error << std::endl;
                //--------------------------
            }// end void handle_error(const char* error)
            //--------------------------
            size_t thread_Workers_size(void) const{
                //--------------------------
                std::unique_lock lock(m_mutex);
                //--------------------------
                return m_workers.size();
                //--------------------------
            }// end size_t thread_Workers_size(void) const
            //--------------------------
            size_t active_tasks_size(void) const {
                //--------------------------
                if constexpr (!static_cast<bool>(use_priority_queue)){
                    std::unique_lock lock(m_mutex);
                }// end if constexpr (!static_cast<bool>(use_priority_queue)){
                //--------------------------
                return m_tasks.size();
                //--------------------------
            }// end size_t ThreadPool::ThreadPool::active_tasks_size(void) const
            //--------------------------
            template <size_t U = adoptive_tick>
            std::enable_if_t<U > 0UL, std::optional<size_t>> safe_increment(const size_t& value) {
                //--------------------------
                if (value == std::numeric_limits<size_t>::max()) {
                    std::cerr << "Maximum Thread IDs have been reached" << std::endl;
                    return std::nullopt;
                }// end if (value == std::numeric_limits<size_t>::max())
                //--------------------------
                return value + 1UL;
                //--------------------------
            }// end std::optional<size_t> safe_increment(const size_t& value)
            //--------------------------
            constexpr std::optional<std::jthread> assign_adoptive_thread(void){
                //--------------------------
                if constexpr (adoptive_tick > 0UL){
                    return std::optional<std::jthread>([this](const std::stop_token& stoken){this->adjustment_thread_function(stoken);});
                }// end if constexpr (adoptive_tick > 0UL)
                //--------------------------
                return std::nullopt;
                //--------------------------
            }// end constexpr std::optional<std::jthread> assign_adoptive_thread(void)
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            const size_t m_upper_threshold;
            //--------------------------
            using WorkersType = std::conditional_t<(adoptive_tick > 0UL), std::unordered_map<size_t, std::jthread>, std::vector<std::jthread>>;
            WorkersType m_workers;
            //--------------------------
            std::optional<std::unordered_set<size_t>> m_idle_threads;
            //--------------------------
            using TaskContainerType = std::conditional_t<static_cast<bool>(use_priority_queue), PriorityQueue<ThreadTask>, std::deque<std::function<void()>>>;
            TaskContainerType m_tasks;
            //--------------------------
            std::optional<std::jthread> m_adoptive_thread;
            //--------------------------
            mutable std::mutex m_mutex;
            //--------------------------
            std::condition_variable m_task_available_condition, m_all_stopped_condition;
            //--------------------------
            // Definitions
            //--------------------------
            static constexpr auto CHECK_INTERVAL = std::chrono::nanoseconds(static_cast<int64_t>(adoptive_tick)); 
            static constexpr size_t m_lower_threshold = 1UL;
        //--------------------------------------------------------------
    };// end class ThreadPool
    //--------------------------------------------------------------
}//end namespace ThreadPool
//--------------------------------------------------------------