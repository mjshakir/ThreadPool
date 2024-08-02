#pragma once 
//--------------------------------------------------------------
// Standard library
//--------------------------------------------------------------
#include <iostream>
#include <functional>
#include <mutex>
#include <any>
#include <future>
#include <compare>
//--------------------------------------------------------------
/**
 * @namespace ThreadPool
 * @brief Contains classes and utilities for thread pool management.
 */
namespace ThreadPool {
    //--------------------------------------------------------------
    /**
     * @class ThreadTask
     * @brief Represents a task to be executed by a thread pool.
     *
     * This class encapsulates a function and its arguments, and provides mechanisms 
     * for executing the function, retrieving the result, and managing priority and retries.
     *
     * @example
     * Example usage:
     * @code
     * ThreadPool::ThreadTask task([](){ return 42; });
     * task.execute();
     * auto future = task.get_future();
     * int result = future.get();
     * @endcode
     */
    class ThreadTask {
        //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------

            //--------------------------------------------------------------
        public:
            //--------------------------
            ThreadTask(void)                          = default;
            //--------------------------
            /**
             * @brief Parameterized constructor to initialize a ThreadTask object.
             *
             * Creates a ThreadTask object from a given function (or callable) and its arguments.
             * The task's priority and retries can also be specified.
             *
             * @tparam Func The callable's type.
             * @tparam Args Variadic template for the callable's argument types.
             * 
             * @param func A callable object representing the function to execute.
             * @param args Arguments to pass to the function.
             * @param priority (Optional) Task's priority. Defaults to 0. Higher values indicate higher priority.
             * @param retries (Optional) Number of times to retry the task in case of failure. Defaults to 0.
             *
             * @note
             * The callable `func` and its arguments `args` are used to internally construct a `std::function` object.
             * The priority and retries allow for flexible task management within a thread pool.
             *
             * @example:
             * @code
             * // A simple function to be passed to the ThreadTask.
             * int add(int a, int b) {
             *     return a + b;
             * }
             *
             * // Create a ThreadTask.
             * ThreadPool::ThreadTask task(add, 3, 4, 5U, 2U);
             * @endcode
             */
            template <typename Func, typename... Args>
            ThreadTask(Func&& func, Args&&... args, uint16_t priority = 0U, uint8_t retries = 0U)
                            :   m_function(init_function(std::forward<Func>(func), std::forward<Args>(args)...)),
                                m_priority(priority),
                                m_retries(retries),
                                m_state(TaskState::PENDING){
                //--------------------------
            }// end ThreadTask(Func&& func, Args&&... args, uint16_t priority = 0u, uint8_t retries = 0u)
            //--------------------------
            ThreadTask(const ThreadTask&)            = delete;
            ThreadTask& operator=(const ThreadTask&) = delete;
            //--------------------------------------------------------------
            ThreadTask(ThreadTask&& other) noexcept;         
            //--------------------------
            ThreadTask& operator=(ThreadTask&& other) noexcept;
            //--------------------------------------------------------------
            std::strong_ordering operator<=>(const ThreadTask& other) const;
            //--------------------------
            /**
             * @brief Executes the task until it's successful or retries run out.
             * 
             * This method will attempt to execute the task. If the execution fails 
             * (throws an exception), it will retry for a pre-set number of times, 
             * decrementing the retry counter with each failure. If all retries are 
             * exhausted, the method returns without any further action.
             * 
             * @note It is the caller's responsibility to ensure enough retries are set 
             * before calling this method if retries are desired.
             *
             * @example
             * ThreadTask task(someFunction, arg1, arg2, 5, 3); // 5 = priority, 3 = retries
             * task.execute();
             */
            void execute(void);
            //--------------------------
            /**
             * @brief Attempts to execute the task once and returns a status.
             * 
             * This method will try to execute the stored function exactly once. If the 
             * function throws any exception, the method returns `false`. Otherwise, it 
             * sets the task state to `COMPLETED` and returns `true`.
             * 
             * @return `true` if the function was executed successfully, `false` otherwise.
             * 
             * @note This function is thread-safe.
             * 
             * @example
             * ThreadTask task(someFunction, arg1, arg2);
             * bool success = task.try_execute();
             * if (success) {
             *     std::cout << "Task executed successfully!" << std::endl;
             * } else {
             *     std::cout << "Task execution failed." << std::endl;
             * }
             */
            bool try_execute(void);
            //--------------------------
            /**
             * @brief Retrieves the future result of a non-void returning task.
             * 
             * This template overload of get_future is meant for tasks 
             * which return a non-void value. If called on a void returning task,
             * a compilation error will be triggered.
             * 
             * @tparam Func The type of the function the task represents.
             * 
             * @return A future representing the result of the task.
             * 
             * @throws std::logic_error If the future has already been retrieved 
             *                          or the task has not been executed yet.
             * 
             * @note This function is thread-safe.
             * 
             * @example
             * ThreadTask task([]() -> int { return 42; });
             * task.execute_local();
             * auto future = task.get_future();
             * std::cout << "Task result: " << future.get() << std::endl; // prints 42
             * @example
             * ThreadTask task([]() { std::cout << "Hello, World!" << std::endl; });
             * // The following line triggers a compile-time error.
             * // auto future = task.get_future<decltype(task)>();
             */
            std::future<std::any> get_future(void);
            //--------------------------
            /**
             * @brief Checks if the task execution is completed.
             * 
             * Determines whether a task has finished its execution. If the task's function 
             * is void returning and has been executed, this function will return true. 
             * If the task returns a non-void value, this function will return true only if 
             * the result has been retrieved using get_future_local().
             * 
             * @return A boolean value indicating whether the task is done.
             * - true: The task is done (either executed for void functions or result retrieved for non-void).
             * - false: The task is still pending or its result hasn't been retrieved yet.
             * 
             * @note This function is thread-safe.
             * 
             * @example
             * ThreadPool::ThreadTask task([]() -> int { return 42; });
             * task.execute_local();
             * std::cout << "Is task done? " << (task.done() ? "Yes" : "No") << std::endl; // prints "No"
             * auto future = task.get_future_local<decltype(task)>();
             * future.get();
             * std::cout << "Is task done? " << (task.done() ? "Yes" : "No") << std::endl; // prints "Yes"
             */
            bool done(void) const;
            //--------------------------
            /**
             * @brief Retrieves the number of retry attempts left for the task.
             * 
             * This function provides the current count of retry attempts remaining for a task.
             * When a task execution fails, the retry count is decremented until it reaches zero,
             * at which point the task will no longer be retried.
             * 
             * @return The number of retry attempts left for the task.
             * 
             * @note This function is thread-safe.
             * 
             * @example
             * ThreadPool::ThreadTask task([]() { //some function
             *  };
             * std::cout << "Retries left: " << static_cast<int>(task.get_retries_local()) << std::endl;
             * // prints the current number of retries left
             */
            uint8_t get_retries(void) const;
            //--------------------------
            /**
             * @brief Retrieves the priority level assigned to the task.
             * 
             * Tasks can be assigned different priority levels to influence their execution order.
             * A higher value indicates a higher priority.
             * 
             * @return The priority level of the task.
             * 
             * @note This function is thread-safe.
             * 
             * @example
             * ThreadPool::ThreadTask task([]() { // some function}, //args,  //priority level);
             * std::cout << "Task priority: " << static_cast<int>(task.get_priority_local()) << std::endl;
             * // prints "Task priority: 5"
             */
            uint16_t get_priority(void) const;
            //--------------------------
            /**
             * @brief Increases the retry count for the task by a specified amount.
             * 
             * Adjusts the number of retry attempts left for the task by adding the provided amount.
             * If the addition results in a value exceeding the maximum limit for uint8_t, 
             * the retry count is set to the maximum value.
             * 
             * @param amount The amount by which to increase the retry count.
             * 
             * @note This function is thread-safe.
             * 
             * @example
             * ThreadPool::ThreadTask task([]() { // some function 
             *  });
             * task.increase_retries(3);  // Increase retries by 3
             */
            void increase_retries(const uint8_t& amount);
            //--------------------------
            /**
             * @brief Decreases the retry count for the task by a specified amount.
             * 
             * Adjusts the number of retry attempts left for the task by subtracting the provided amount.
             * If the subtraction results in a value below 0, the retry count is set to 0.
             * 
             * @param amount The amount by which to decrease the retry count.
             * 
             * @note This function is thread-safe.
             * 
             * @example
             * ThreadPool::ThreadTask task([]() { // some function 
             * });
             * task.decrease_retries(2);  // Decrease retries by 2
             */
            void decrease_retries(const uint8_t& amount);
            //--------------------------
            /**
             * @brief Increases the retry count for the task by 1.
             * 
             * This is a convenience function that calls increase_retries with an argument of 1.
             * 
             * @note This function is thread-safe.
             * 
             * @example
             * ThreadPool::ThreadTask task([]() { // some function 
             * });
             * task.increase_retries();  // Increase retries by 1
             */
            void increase_retries(void);
            //--------------------------
            /**
             * @brief Decreases the retry count for the task by 1.
             * 
             * This is a convenience function that calls decrease_retries with an argument of 1.
             * 
             * @note This function is thread-safe.
             * 
             * @example
             * ThreadPool::ThreadTask task([]() { // some function 
             * });
             * task.decrease_retries();  // Decrease retries by 1
             */
            void decrease_retries(void);
            //--------------------------
            /**
             * @brief Increases the priority of the task by a specified amount in a thread-safe manner.
             * 
             * Adjusts the priority of the task by adding the provided amount.
             * If the addition results in a value exceeding the maximum limit for uint8_t, 
             * the priority is set to the maximum value.
             * 
             * @note This function is thread-safe.
             * 
             * @param amount The amount by which to increase the priority.
             * 
             * @example
             * ThreadPool::ThreadTask task([]() { // some function
             *   });
             * task.increase_priority(3);  // Increase priority by 3
             */
            void increase_priority(const uint16_t& amount);
            //--------------------------
            /**
             * @brief Decreases the priority of the task by a specified amount in a thread-safe manner.
             * 
             * Adjusts the priority of the task by subtracting the provided amount.
             * If the subtraction results in a value below 0, the priority is set to 0.
             * 
             * @note This function is thread-safe.
             * 
             * @param amount The amount by which to decrease the priority.
             * 
             * @example
             * ThreadPool::ThreadTask task([]() { // some function 
             * });
             * task.decrease_priority(2);  // Decrease priority by 2
             */
            void decrease_priority(const uint16_t& amount);
            //--------------------------
            /**
             * @brief Increases the priority of the task by 1 in a thread-safe manner.
             * 
             * This is a convenience function that calls increase_priority_local with an argument of 1.
             * 
             * @note This function is thread-safe.
             * 
             * @example
             * ThreadPool::ThreadTask task([]() {some function
             *  });
             * task.increase_priority();  // Increase priority by 1
             */
            void increase_priority(void);
            //--------------------------
            /**
             * @brief Decreases the priority of the task by 1 in a thread-safe manner.
             * 
             * This is a convenience function that calls decrease_priority_local with an argument of 1.
             * 
             * @note This function is thread-safe.
             * 
             * @example
             * ThreadPool::ThreadTask task([]() { some function 
             * });
             * task.decrease_priority();  // Decrease priority by 1
             */
            void decrease_priority(void);
            //--------------------------
            /**
             * @brief Retrieves the current status of the task in a thread-safe manner.
             * 
             * This method provides a way to inspect the state of a task, whether it's pending, completed, or its result has been retrieved.
             * 
             * @note This function is thread-safe.
             * 
             * @return An unsigned 8-bit integer representing the task's state. 
             *         - 0: PENDING
             *         - 1: COMPLETED
             *         - 2: RETRIEVED
             * 
             * @example
             * ThreadPool::ThreadTask task([]() { // some function  
             * });
             * auto status = task.get_status();
             * std::cout << "Task status: " << (status == 0 ? "PENDING" : (status == 1 ? "COMPLETED" : "RETRIEVED")) << std::endl;
             */
            uint8_t get_status(void) const;
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            template<typename Func>
            static constexpr bool is_void_function(void) {
                return std::is_same_v<std::invoke_result_t<Func>, void>;
            }// end constexpr bool is_void_function(void)
            //--------------------------------------------------------------
            template <typename Func, typename... Args>
            auto init_function(Func&& func, Args&&... args) -> std::function<std::any()> {
                //--------------------------    
                auto shared_func = std::make_shared<Func>(std::move(func));
                //--------------------------
                return [shared_func, ...capturedArgs = std::forward<Args>(args)]() mutable -> std::any {
                    //--------------------------
                    if constexpr (is_void_function<Func>()) {
                        std::invoke(*shared_func, std::forward<decltype(capturedArgs)>(capturedArgs)...);
                        return std::any{};
                    } else {
                        return std::invoke(*shared_func, std::forward<decltype(capturedArgs)>(capturedArgs)...);
                    }
                    //--------------------------
                };
                //--------------------------
            }// end auto init_function(Func&& func, Args&&... args) -> std::function<std::any()> 
            //--------------------------
            void execute_local(void);
            //--------------------------
            bool try_execute_local(void);
            //--------------------------
            template<typename Func>
            std::enable_if_t<!is_void_function<Func>(), std::future<std::any>>
            get_future_local(void) {
                //--------------------------
                std::lock_guard<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_state == TaskState::RETRIEVED) {
                    throw std::logic_error("Future already retrieved!");
                }
                //--------------------------
                if (m_state == TaskState::PENDING) {
                    throw std::logic_error("Task not yet executed!");
                }
                //--------------------------
                m_state = TaskState::RETRIEVED;
                //--------------------------
                return m_promise.get_future();
                //--------------------------
            }// end get_future_local(void)
            //--------------------------
            template<typename Func>
            std::enable_if_t<std::is_void_v<Func>>
            get_future_local(void) {
                //--------------------------
                static_assert(is_void_function<Func>(), "Cannot get future from a void function!");
                //--------------------------
            }// end get_future_local(void) 
            //--------------------------
            bool is_done(void) const;
            //--------------------------
            uint8_t get_retries_local(void) const;
            //--------------------------
            uint16_t get_priority_local(void) const;
            //--------------------------
            void increase_retries_local(const uint8_t& amount);
            //--------------------------
            void decrease_retries_local(const uint8_t& amount);
            //--------------------------
            void increase_priority_local(const uint16_t& amount);
            //--------------------------
            void decrease_priority_local(const uint16_t& amount);
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            std::function<std::any()> m_function;
            uint16_t m_priority;
            uint8_t m_retries;
            //--------------------------
            enum class TaskState : uint8_t {
                PENDING = 0, COMPLETED, RETRIEVED
            };
            //--------------------------
            TaskState m_state;
            //--------------------------
            std::promise<std::any> m_promise;
            mutable std::mutex m_mutex;
        //--------------------------------------------------------------
    };// end class ThreadTask
    //--------------------------------------------------------------
}//end namespace ThreadPool
//--------------------------------------------------------------