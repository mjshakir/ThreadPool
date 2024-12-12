#pragma once 

//--------------------------------------------------------------
// Standard library
//--------------------------------------------------------------
#include <iostream>
#include <memory>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "ThreadPool.hpp"
//--------------------------------------------------------------
namespace ThreadPool {
    //--------------------------------------------------------------
    enum class PrecedenceLevel : uint8_t {
        LOW = 0U, MEDIUM, HIGH
    }; // end enum class PrecedenceLevel
    //--------------------------------------------------------------
    class ThreadPoolManager {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            // Get the global ThreadPoolManager instance
            static ThreadPoolManager& get_instance(void);
            //--------------------------
            // Set the mode and tick for the global ThreadPool. Must be called once before accessing the ThreadPool.
            template <ThreadMode Mode, size_t Tick, PrecedenceLevel Precedence>
            constexpr bool configure(const size_t& number_threads = 0UL) {
                bool _adaptive = (Tick > 0UL);
                if (m_instance) {
                    if (should_override_configuration(Mode, _adaptive, Precedence)) {
                        //--------------------------
                        m_instance = std::move(std::unique_ptr<ThreadPool<>>(new ThreadPool<Mode, Tick>(number_threads)));
                        update_configuration(Mode, _adaptive, Precedence);
                        return true;
                        //--------------------------
                    } // end if (should_override_configuration(Mode, _adaptive, Precedence))
                    //--------------------------
                    return false;
                    //--------------------------
                } // end if (m_instance)
                //--------------------------
                m_instance = std::move(std::unique_ptr<ThreadPool<>>(new ThreadPool<Mode, Tick>(number_threads)));
                update_configuration(Mode, _adaptive, Precedence);
                //--------------------------
                return true;
                //--------------------------
            } // end bool configure(void)
            //--------------------------
            // Check if the ThreadPool is already initialized
            bool initialized(void) const;
            //--------------------------
            // Get the global ThreadPool instance
            ThreadPool<>& get_thread_pool(void) const;
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            constexpr bool should_override_configuration(const ThreadMode& mode, bool adaptive_tick, const PrecedenceLevel& precedence) const;
            constexpr void update_configuration(const ThreadMode& mode, bool adaptive_tick, const PrecedenceLevel& precedence);
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            // Constructor using initializer list for efficiency
            ThreadPoolManager(void);
            ~ThreadPoolManager(void) = default;
            //--------------------------
            std::unique_ptr<ThreadPool<>> m_instance; // Managed uniquely within ThreadPoolManager
            ThreadMode m_current_mode;
            bool m_adaptive_tick;
            PrecedenceLevel m_current_precedence;
        //--------------------------------------------------------------
    }; // end class ThreadPoolManager
    //--------------------------------------------------------------
} // end namespace ThreadPool
//--------------------------------------------------------------