#pragma once
//--------------------------------------------------------------
// Standard library
//--------------------------------------------------------------
#include <thread>
#include <cstddef>
#include <cstdbool>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "ThreadMode.hpp"
#include "ThreadPool.hpp"
//--------------------------------------------------------------
/** @namespace ThreadPool
 * @brief A namespace containing the ThreadPoolManager class.
 */
namespace ThreadPool {
    //--------------------------------------------------------------
    class ThreadPoolManager {
        //--------------------------------------------------------------
        private:
        //--------------------------------------------------------------
        // If THREADPOOL_MODE is 1 => PRIORITY, else STANDARD
#if THREADPOOL_MODE
            static constexpr ThreadMode C_THREADMODE    = ThreadMode::PRIORITY;
#else
            static constexpr ThreadMode C_THREADMODE    = ThreadMode::STANDARD;
#endif
            static constexpr size_t C_ADOPTIVE_TICK     = THREADPOOL_ADOPTIVE_TICK;
            //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            /**
             * @brief Get the instance of the ThreadPoolManager
             * 
             * @param number_threads Number of threads to be created in the ThreadPool
             * 
             * @return ThreadPoolManager& Reference to the ThreadPoolManager instance. @default: number of threads is the number of hardware threads.
             * 
             * @note This function is thread-safe
             * 
             * @example
             * 
             * @code
             * 
             * #include <iostream>
             * #include "ThreadPoolManager.hpp"
             * 
             * int main() {
             *      // Get the instance of the ThreadPoolManager
             *      ThreadPool::ThreadPoolManager& manager = ThreadPool::ThreadPoolManager::get_instance();
             * 
             * return 0;
             * }
             * 
             * @endcode
             */
            static ThreadPoolManager& get_instance(const size_t& number_threads = static_cast<size_t>(std::thread::hardware_concurrency()));
            //--------------------------
            /**
             * @brief Get the ThreadPool instance
             * 
             * @details This function returns a reference to the ThreadPool instance managed by the ThreadPoolManager.
             * The ThreadPool instance is created with the specified number of threads during the instantiation of the ThreadPoolManager.
             * The CMAKE function threadpool_request_config can be used to be created in the ThreadPool with differnt mode and tick in any CMAKE project.
             * 
             * @return ThreadPool<ThreadPoolManager::C_THREADMODE, ThreadPoolManager::C_ADOPTIVE_TICK>& Reference to the ThreadPool instance
             * 
             * @example
             * 
             * @code
             * 
             * #include <iostream>
             * #include "ThreadPoolManager.hpp"
             * 
             * int main() {
             *      // Get the instance of the ThreadPoolManager 
             *      auto& manager = ThreadPool::ThreadPoolManager::get_instance().get_thread_pool();
             * 
             *      std::cout   << "ThreadPoolManager Singleton" 
             *                  << " | ThreadMode mode: " << ThreadPool::ThreadMode_name(_thread_pool.mode())
             *                  << " | Adoptive: " << std::boolalpha << _thread_pool.adoptive()
             *                  << " | Adoptive Tick: " << _thread_pool.adoptive_tick_size() << std::endl;
             * 
             * return 0;
             * }
             * 
             * @endcode
             */
            ThreadPool<C_THREADMODE, C_ADOPTIVE_TICK>& get_thread_pool(void) {
                return m_thread_pool;
            }// end ThreadPool<ThreadPoolManager::C_THREADMODE, ThreadPoolManager::C_ADOPTIVE_TICK>& get_thread_pool(void)
            //--------------------------
            /**
             * @brief Get the ThreadPool Mode
             * 
             * @return ThreadMode Enum value representing the ThreadPool mode
             * 
             * @example
             * 
             * @code
             * 
             * #include <iostream>
             * #include "ThreadPoolManager.hpp"
             * #inlcude "ThreadMode.hpp"
             * 
             * int main() {
             *      // Get the instance of the ThreadPoolManager
             *      // auto& manager = ThreadPool::ThreadPoolManager::get_instance();
             * 
             *      std::cout << "ThreadPool Mode: " << ThreadPool::ThreadMode_name(ThreadPool::ThreadPoolManager::mode()) << std::endl;
             * 
             * return 0;
             * }
             * 
             * @endcode
             */
            static constexpr ThreadMode mode(void) {
                return C_THREADMODE;
            }// end constexpr ThreadMode thread_mode(void)
            //--------------------------
            /**
             * @brief Get adaptive tick size
             * 
             * @return size_t The adaptive tick size
             * 
             * @example
             * 
             * @code
             * 
             * #include <iostream>
             * #include "ThreadPoolManager.hpp"
             * 
             * int main() {
             *      // Get the instance of the ThreadPoolManager
             *      auto& manager = ThreadPool::ThreadPoolManager::get_instance();
             * 
             *      std::cout << "ThreadPool adoptive_tick: " << manager.number_threads() << std::endl;
             * 
             * return 0;
             * }
             * 
             * @endcode
             */
            static constexpr size_t adoptive_tick(void) {
                return C_ADOPTIVE_TICK;
            }// end constexpr size_t adoptive_tick(void)
            //--------------------------
            /**
             * @brief Get if the ThreadPool is adoptive
             * 
             * @return bool True if the ThreadPool is adoptive, false otherwise
             * 
             * @example
             * 
             * @code
             * 
             * #include <iostream>
             * #include "ThreadPoolManager.hpp"
             * 
             * int main() {
             *     // Get the instance of the ThreadPoolManager
             *     // auto& manager = ThreadPool::ThreadPoolManager::get_instance();
             * 
             *    std::cout << "ThreadPool adoptive: " << std::boolalpha << ThreadPool::ThreadPoolManager::adoptive() << std::endl;
             * 
             * return 0;
             * }
             * 
             * @endcode
             */
            static constexpr bool adoptive(void) {
                return 0 < C_ADOPTIVE_TICK;
            }// end constexpr bool adoptive(void)
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            ThreadPoolManager(void)                                 = delete;
            //--------------------------
            explicit ThreadPoolManager(const size_t& number_threads);
            //--------------------------
            ~ThreadPoolManager(void)                                = default;
            //--------------------------
            // Delete move and copy constructor and assignment operator to ensure singleton properties
            //--------------------------
            ThreadPoolManager(ThreadPoolManager&&)                  = delete;
            ThreadPoolManager(const ThreadPoolManager&)             = delete;
            ThreadPoolManager& operator=(ThreadPoolManager&&)       = delete;
            ThreadPoolManager& operator=(const ThreadPoolManager&)  = delete;
            //--------------------------
            ThreadPool<C_THREADMODE, C_ADOPTIVE_TICK> m_thread_pool;
        //--------------------------------------------------------------
    };//end class ThreadPoolManager
    //--------------------------------------------------------------
}//end namespace ThreadPool
//--------------------------------------------------------------