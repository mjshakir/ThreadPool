#pragma once 
//--------------------------------------------------------------
// Standard library
//--------------------------------------------------------------
#include <unordered_set>
#include <mutex>
#include <thread>
#include <memory>
#include <optional>
//--------------------------------------------------------------
namespace ThreadPool{
    //--------------------------------------------------------------
    class ThreadSafeSet {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            ThreadSafeSet(void) = default;
            //--------------------------
            ThreadSafeSet(const size_t& size = static_cast<size_t>(std::thread::hardware_concurrency()));
            //--------------------------
            void add(const std::shared_ptr<std::jthread>& thread);
            //--------------------------
            void remove(const std::shared_ptr<std::jthread>& thread);
            //--------------------------
            bool empty(void) const;
            //--------------------------
            void reserve(const size_t& size);
            //--------------------------
            std::optional<std::shared_ptr<std::jthread>> get_thread(void);
            //--------------------------------------------------------------
            protected:
            //--------------------------------------------------------------
            void add_thread(const std::shared_ptr<std::jthread>& thread);
            //--------------------------
            void remove_thread(const std::shared_ptr<std::jthread>& thread);
            //--------------------------
            bool empty_set(void) const;
            //--------------------------
            void reserve_size(const size_t& size);
            //--------------------------
            std::optional<std::shared_ptr<std::jthread>> get_thread_local(void);
            //--------------------------------------------------------------
        private:
            std::unordered_set<std::shared_ptr<std::jthread>> m_set;
            mutable std::mutex m_mutex;
        //--------------------------------------------------------------
    }; // end class ThreadSafeSet
    //--------------------------------------------------------------
}//end namespace ThreadPool