#pragma once 
//--------------------------------------------------------------
// Standard library
//--------------------------------------------------------------
#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <functional>
#include <optional>
#include <mutex>
#include <type_traits>
//--------------------------------------------------------------
/**
 * @namespace ThreadPool
 * @brief Contains classes and utilities for thread pool management.
 */
namespace ThreadPool{
    //--------------------------------------------------------------
    /**
     * @class PriorityQueue
     * @brief A thread-safe priority queue tailored for use in a thread pool.
     *
     * This class provides a thread-safe implementation of a priority queue with additional features 
     * for handling custom tasks in a thread pool context. The priority of the tasks is determined 
     * by the Comparator provided to the queue.
     *
     * @tparam T The type of elements in the queue.
     * @tparam Container The underlying container to use (defaults to std::vector<T>).
     * @tparam Comparator The comparator to determine priority (defaults to std::less).
     */
    template<typename T, typename Container = std::vector<T>, typename Comparator = std::less<typename Container::value_type>>
    class PriorityQueue {
        //--------------------------------------------------------------
        // Check if the Container type is either std::vector<T> or std::deque<T>
        //--------------------------
        static_assert(std::disjunction_v<std::is_same<Container, std::vector<T>>, std::is_same<Container, std::deque<T>>>,
                  "Container should be either std::vector or std::deque");
        //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            template <typename U, typename = void>
            struct has_is_done : std::false_type {};
            //--------------------------
            template <typename U>
            struct has_is_done<U, std::void_t<decltype(std::declval<U>().is_done())>> 
            : std::integral_constant<bool, std::is_same_v<decltype(std::declval<U>().is_done()), bool>> {};
            //--------------------------
            template <typename U, typename = void>
            struct has_done : std::false_type {};
            //--------------------------
            template <typename U>
            struct has_done<U, std::void_t<decltype(std::declval<U>().done())>> 
            : std::integral_constant<bool, std::is_same_v<decltype(std::declval<U>().done()), bool>> {};
            //--------------------------
            template <typename U>
            struct has_either_is_done_or_done : std::disjunction<has_is_done<U>, has_done<U>> {};
            //--------------------------------------------------------------
        public:
            PriorityQueue(void)                            = default;
            //--------------------------------------------------------------
            PriorityQueue(const PriorityQueue& other)
                :   m_data([&]() { 
                        std::lock_guard<std::mutex> lock(other.m_mutex);
                        return other.m_data;
                    }()),
                    m_comp([&]() { 
                        std::lock_guard<std::mutex> lock(other.m_mutex);
                        return other.m_comp;
                    }()) {
                //--------------------------
            }// end PriorityQueue(const PriorityQueue& other)
            //--------------------------
            PriorityQueue& operator=(const PriorityQueue& other){
                //--------------------------
                if (&other == this) {
                    return *this;
                }// end if (&other == this)
                //--------------------------
                std::scoped_lock lock(m_mutex, other.m_mutex); // Locks both mutexes safely
                //--------------------------
                m_data = other.m_data;
                m_comp = other.m_comp;
                //--------------------------
                return *this;
                //--------------------------
            }// end PriorityQueue& operator=(const PriorityQueue& other)
            //--------------------------------------------------------------
            PriorityQueue(PriorityQueue&& other) noexcept
                :   m_data([&]() { 
                        std::lock_guard<std::mutex> lock(other.m_mutex);
                        return std::move(other.m_data);
                    }()),
                    m_comp([&]() { 
                        std::lock_guard<std::mutex> lock(other.m_mutex);
                        return std::move(other.m_comp);
                    }()) {
                //--------------------------
            }// end PriorityQueue(PriorityQueue&& other) noexcept
            //--------------------------
            PriorityQueue& operator=(PriorityQueue&& other) noexcept {
                //--------------------------
                if (&other == this) {
                    return *this;
                }// end if (&other == this) 
                //--------------------------
                std::scoped_lock lock(m_mutex, other.m_mutex); // Locks both mutexes safely
                //--------------------------
                m_data = std::move(other.m_data);
                m_comp = std::move(other.m_comp);
                //--------------------------
                return *this;
                //--------------------------
            }// end PriorityQueue& operator=(PriorityQueue&& other) noexcept
            //--------------------------------------------------------------
            /**
             * @brief Checks if the priority queue is empty.
             * 
             * This function atomically checks whether the underlying container of the 
             * priority queue has any elements. 
             * 
             * @tparam T Type of elements in the priority queue.
             * 
             * @return bool 
             *         Returns true if the queue is empty, and false otherwise.
             * 
             * @example
             * 
             * PriorityQueue<int> queue;
             * 
             * if (queue.empty()) {
             *     std::cout << "Queue is empty!" << std::endl;
             * } else {
             *     std::cout << "Queue is not empty." << std::endl;
             * }
             * 
             * @note This function is thread-safe and can be invoked concurrently by 
             *       multiple threads without external synchronization.
             */
            bool empty(void) const {
                return is_empty();
            }// end bool empty(void) const
            //--------------------------
            /**
             * @brief Reserves space in the underlying container of the priority queue.
             * 
             * This function atomically attempts to preallocate enough memory for 
             * the underlying container to hold the specified number of elements. 
             * This can be beneficial in situations where you know in advance 
             * about the approximate number of elements the priority queue will hold.
             * 
             * @tparam T Type of elements in the priority queue.
             * 
             * @param size Number of elements the container should have the capacity for.
             * 
             * @example
             * 
             * ThreadPool::PriorityQueue<int> queue;
             * 
             * // Reserve space for 1000 elements
             * queue.reserve(1000);
             * 
             * @note This function is thread-safe and can be invoked concurrently by 
             *       multiple threads without external synchronization.
             * 
             * @warning This only affects capacity, not the actual number of elements in 
             *          the queue. The size of the queue remains unchanged.
             */
            template<typename U = Container, std::enable_if_t<std::is_same_v<U, std::vector<T>>, int> = 0>
            void reserve(const size_t& size) {
                reserve_m_data(size);
            }// endvoid reserve(const size_t& size)
            //--------------------------
            /**
             * @brief Atomically adds an element to the priority queue.
             * 
             * This function inserts the given element into the priority queue 
             * maintaining the heap property. It ensures that the priority queue 
             * remains correctly sorted and that the top element is always the one 
             * with the highest priority as determined by the comparator `m_comp`.
             * 
             * @tparam T Type of elements in the priority queue.
             * 
             * @param value The element to be added to the priority queue.
             * 
             * @example
             * 
             * ThreadPool::PriorityQueue<int> queue;
             * 
             * // Push a value into the priority queue
             * queue.push(42);
             * 
             * @note This function is thread-safe and can be invoked concurrently by 
             *       multiple threads without external synchronization.
             * 
             */
            void push(const T& value){
                push_internal(value);
            }// end void push(const T& value)
            //--------------------------
            /**
             * @brief Atomically adds an element to the priority queue using move semantics.
             * 
             * This function inserts the given element into the priority queue 
             * maintaining the heap property by effectively moving it, rather than 
             * copying. This is especially beneficial for objects that are expensive 
             * to copy. It ensures that the priority queue remains correctly sorted 
             * and that the top element is always the one with the highest priority 
             * as determined by the comparator `m_comp`.
             * 
             * @tparam T Type of elements in the priority queue.
             * 
             * @param value The element to be added to the priority queue. The content of 
             *              `value` is moved into the queue, and `value` is left in a 
             *              valid but unspecified state.
             * 
             * @example
             * 
             * ThreadPool::PriorityQueue<std::vector<int>> queue;
             * 
             * // Create a vector and move it into the priority queue
             * std::vector<int> vec = {1, 2, 3};
             * queue.push(std::move(vec));
             * 
             * // After the push, vec is left in an unspecified but valid state
             * 
             * @note This function is thread-safe and can be invoked concurrently by 
             *       multiple threads without external synchronization.
             */
            void push(T&& value){
                push_internal(std::move(value));
            }// end void push(const T& value)
            //--------------------------
            /**
             * @brief Constructs an element in-place at the end of the priority queue.
             * 
             * This function constructs an element in the priority queue without having 
             * to create a temporary object, using the provided arguments. The constructed 
             * element is then correctly positioned in the queue based on the comparator 
             * `m_comp`.
             * 
             * @tparam Args Variadic template representing the types of the arguments to 
             *              construct the element of type `T`.
             * 
             * @param args Arguments to be forwarded to the constructor of the element.
             * 
             * @example
             * 
             * ThreadPool::PriorityQueue<std::pair<std::string, int>> queue;
             * 
             * // Emplace a pair directly into the priority queue
             * std::string key = "sample";
             * int value = 42;
             * queue.emplace(key, value);
             * 
             * @note This function is thread-safe and can be invoked concurrently by 
             *       multiple threads without external synchronization.
             */
            template<typename... Args>
            void emplace(Args&&... args) {
                emplace_internal(std::forward<Args>(args)...);
            }// end void emplace(Args&&... args)
            //--------------------------
            /**
             * @brief Fetches the top element of the priority queue without removing it.
             * 
             * This function safely retrieves the top (highest priority) element from 
             * the priority queue without removing it. If the queue is empty, 
             * a `std::nullopt` is returned, indicating the absence of any element.
             * 
             * @return A `std::optional` containing the top element if the queue is 
             *         not empty; otherwise, returns `std::nullopt`.
             * 
             * @example
             * 
             * ThreadPool::PriorityQueue<int> queue;
             * queue.push_internal(5);
             * queue.push_internal(10);
             * queue.push_internal(15);
             * 
             * // Fetch the top element
             * auto topElement = queue.top();
             * if (topElement.has_value()) {
             *     std::cout << "Top element: " << topElement.value() << std::endl; // Outputs: 15
             * } else {
             *     std::cout << "Queue is empty." << std::endl;
             * }
             * 
             * @note This function is thread-safe and can be invoked concurrently by 
             *       multiple threads without external synchronization.
             * 
             */
            std::optional<T> top(void) const {
                return top_internal();
            }// end std::optional<T> top() const
            //--------------------------
            /**
             * @brief Safely removes the top element from the priority queue.
             * 
             * This function ensures thread-safe removal of the top (highest priority) 
             * element from the priority queue. If the queue is empty, the function 
             * simply returns without making any changes.
             * 
             * @example
             * 
             * ThreadPool::PriorityQueue<int> queue;
             * queue.push_internal(5);
             * queue.push_internal(10);
             * queue.push_internal(15);
             * 
             * std::cout << "Size before pop: " << queue.size() << std::endl; // Outputs: 3
             * 
             * // Pop the top element
             * queue.pop();
             * 
             * std::cout << "Size after pop: " << queue.size() << std::endl; // Outputs: 2
             * 
             * @note This function is thread-safe and can be invoked concurrently by 
             *       multiple threads without external synchronization.
             */
            void pop(void) {
                pop_internal();
            }// end void pop(void)
            //--------------------------
            /**
             * @brief Pops the top element from the priority queue and returns it.
             * 
             * This function atomically removes the highest priority element from the 
             * queue (based on the Comparator used during instantiation) and returns it.
             * The function utilizes `std::pop_heap` to maintain the heap invariant of the
             * underlying container.
             * 
             * @tparam T Type of elements in the priority queue.
             * 
             * @return std::optional<T> 
             *         If the queue is not empty, returns an optional containing the top element.
             *         If the queue is empty, returns an empty optional (std::nullopt).
             * 
             * @example
             * 
             * PriorityQueue<int> queue;
             * queue.push(1);
             * queue.push(2);
             * queue.push(3);
             * 
             * auto top = queue.pop_top();
             * if (top) {
             *     std::cout << "Popped top element: " << *top << std::endl;
             * } else {
             *     std::cout << "Queue is empty!" << std::endl;
             * }
             * 
             * @note This function is thread-safe and can be invoked concurrently by 
             *       multiple threads without external synchronization.
             */
            std::optional<T> pop_top(void) {
                return pop_top_internal();
            }// end std::optional<T> pop_top(void)
            //--------------------------
            /**
             * @brief Retrieves the current size of the priority queue in a thread-safe manner.
             * 
             * This function provides a thread-safe way to determine the number of elements
             * currently present in the priority queue.
             * 
             * @example
             * 
             * ThreadPool::PriorityQueue<int> queue;
             * queue.push_internal(1);
             * queue.push_internal(2);
             * 
             * std::cout << "Size of the queue: " << queue.size_local() << std::endl; // Outputs: 2
             * 
             * @note This function is thread-safe and can be invoked concurrently by 
             *       multiple threads without external synchronization.
             * 
             * @return The number of elements currently present in the priority queue.
             */
            size_t size(void) const {
                return size_local();
            }// end constexpr size_t size(void)
            //--------------------------
            template<typename U = T, std::enable_if_t<!has_either_is_done_or_done<U>::value, int> = 0>
            void remove(void) = delete;
            //--------------------------
            /**
            * @brief Removes all tasks that are done from the priority queue.
            * 
            * This function provides a mechanism to purge finished tasks from the 
            * priority queue in a thread-safe manner. The definition of a "finished task"
            * is determined by whether the task type has a `is_done()` or `done()` member function.
            * 
            * @tparam U The task type. Defaults to `T`.
            * 
            * @example
            * 
            * // Suppose TaskType is the type stored in the PriorityQueue and it has a `is_done()` member function.
            * ThreadPool::PriorityQueue<TaskType> queue;
            * // ... [populate queue]
            * 
            * queue.remove();  // Will invoke appropriate specialization to remove "done" tasks.
            * 
            * @note This function internally determines which version (based on the presence 
            *       of `is_done()` or `done()`) to call and removes tasks accordingly.
            * 
            * @note This function is thread-safe and can be invoked concurrently by 
            *       multiple threads without external synchronization.
            * 
            * @warning If the task type `U` doesn't have either of the `is_done()` or `done()` 
            *          member functions, invoking this function will result in a compilation error.
            */
            template<typename U = T, std::enable_if_t<has_either_is_done_or_done<U>::value, int> = 0>
            void remove(void) {
                remove_tasks<has_is_done<U>::value, has_done<U>::value>();
            }// end void remove(void)
            //--------------------------
            /**
             * @brief Removes a specific task from the priority queue.
             * 
             * This function provides a mechanism to remove a specific task from the 
             * priority queue. The removal process is based on value comparison. 
             * After removing the task, the remaining tasks are restructured to maintain 
             * the properties of the priority queue.
             * 
             * @param[in] value The task to be removed from the priority queue.
             * 
             * @example
             * 
             * // Suppose TaskType is the type stored in the PriorityQueue.
             * ThreadPool::PriorityQueue<TaskType> queue;
             * 
             * TaskType task1;
             * TaskType task2;
             * // ... [populate queue with task1, task2, ...]
             * 
             * queue.remove(task1);  // task1 will be removed from the queue.
             * 
             * @note The removal process is done in a thread-safe manner.
             * @note If the task does not exist in the queue or there are multiple tasks with 
             *       the same value, only the first occurrence will be removed.
             */
            void remove(const T& value){
                remove_task(value);
            }// end void remove(const T& value)
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            bool is_empty(void) const {
                std::lock_guard<std::mutex> lock(m_mutex);
                return m_data.empty();
            }// end bool is_empty() const
            //--------------------------
            template<typename U = Container, std::enable_if_t<std::is_same_v<U, std::vector<T>>, int> = 0>
            void reserve_m_data(const size_t& size) {
                //--------------------------
                std::lock_guard<std::mutex> lock(m_mutex);
                m_data.reserve(size);
                //--------------------------
            }// end void reserve_m_data(const size_t& size)
            //--------------------------
            void push_internal(const T& value) {
                //--------------------------
                std::lock_guard<std::mutex> lock(m_mutex);
                m_data.push_back(value);
                std::push_heap(m_data.begin(), m_data.end(), m_comp);
                //--------------------------
            }// end void push_internal(const T& value)
            //--------------------------
            void push_internal(T&& value) {
                //--------------------------
                std::lock_guard<std::mutex> lock(m_mutex);
                m_data.push_back(std::move(value));
                std::push_heap(m_data.begin(), m_data.end(), m_comp);
                //--------------------------
            }// end void push_internal(const T& value)
            //--------------------------
            template<typename... Args>
            void emplace_internal(Args&&... args) {
                //--------------------------
                std::lock_guard<std::mutex> lock(m_mutex);
                m_data.emplace_back(std::forward<Args>(args)...);
                std::push_heap(m_data.begin(), m_data.end(), m_comp);
                //--------------------------
            }// end void emplace_internal(Args&&... args)
            //--------------------------
            std::optional<T> top_internal(void) const {
                //--------------------------
                std::lock_guard<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_data.empty()) {
                    return std::nullopt;
                }// end if (m_data.empty())
                //--------------------------
                return m_data.front();
                //--------------------------
            }// end std::optional<T> top_internal(void) const
            //--------------------------
            void pop_internal(void) {
                //--------------------------
                std::lock_guard<std::mutex> lock(m_mutex);
                //--------------------------
                if (!m_data.empty()) {
                    //--------------------------
                    std::pop_heap(m_data.begin(), m_data.end(), m_comp);
                    m_data.pop_back();
                    //--------------------------
                }// end if (!m_data.empty())
                //--------------------------   
            }// end void pop_internal(void)
            //--------------------------
            std::optional<T> pop_top_internal(void) {
                //--------------------------
                std::lock_guard<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_data.empty()) {
                    return std::nullopt;
                }// end if (m_data.empty())
                //--------------------------
                T top_element = std::move(m_data.front());
                std::pop_heap(m_data.begin(), m_data.end(), m_comp);
                m_data.pop_back();
                return top_element;
                //--------------------------
            }// end std::optional<T> pop_top_internal(void)
            //--------------------------
            size_t size_local(void) const{
                //--------------------------
                std::lock_guard<std::mutex> lock(m_mutex);
                //--------------------------
                return m_data.size();
                //--------------------------
            }// end constexpr size_t size(void)
            //--------------------------
            template<bool UseIsDone, bool UseDone, typename std::enable_if_t<UseIsDone, int> = 0>
            void remove_tasks(void) {
                //--------------------------
                std::lock_guard<std::mutex> lock(m_mutex);
                //--------------------------
                auto partition_point = std::partition(m_data.begin(), m_data.end(), [](const T& task) {
                    return !task.is_done();
                });
                //--------------------------
                m_data.erase(partition_point, m_data.end());
                std::make_heap(m_data.begin(), m_data.end(), m_comp);
                //--------------------------
            }// end void remove_tasks(void)
            //--------------------------
            template<bool UseIsDone, bool UseDone, typename std::enable_if_t<!UseIsDone and UseDone, int> = 0>
            void remove_tasks(void) {
                //--------------------------
                std::lock_guard<std::mutex> lock(m_mutex);
                //--------------------------
                auto partition_point = std::partition(m_data.begin(), m_data.end(), [](const T& task) {
                    return !task.done();
                });
                //--------------------------
                m_data.erase(partition_point, m_data.end());
                std::make_heap(m_data.begin(), m_data.end(), m_comp);
                //--------------------------
            }// end void remove_tasks(void) 
            //--------------------------
            void remove_task(const T& value) {
                //--------------------------
                std::lock_guard<std::mutex> lock(m_mutex);
                //--------------------------
                auto new_end = std::remove(m_data.begin(), m_data.end(), value);
                m_data.erase(new_end, m_data.end());
                std::make_heap(m_data.begin(), m_data.end(), m_comp);
                //--------------------------
            }// end void remove_task(const T& value)
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            Container m_data;
            Comparator m_comp;
            mutable std::mutex m_mutex;            
        //--------------------------------------------------------------
    };// end class PriorityQueue 
    //--------------------------------------------------------------
}//end namespace ThreadPool
//--------------------------------------------------------------