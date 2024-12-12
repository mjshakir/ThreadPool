#pragma once 

//--------------------------------------------------------------
// Standard library
//--------------------------------------------------------------
#include <iostream>
#include <memory>
#include <any>
#include <thread>
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
            constexpr bool configure(const size_t& number_threads = static_cast<size_t>(std::thread::hardware_concurrency())) {
                bool _adaptive = (Tick > 0UL);
                if (m_instance) {
                    if (reconfiguration(Mode, _adaptive, Precedence)) {
                        //--------------------------
                        m_instance = std::make_unique<std::any>(ThreadPool<Mode, Tick>(number_threads));
                        update_configuration(Mode, _adaptive, Precedence);
                        return true;
                        //--------------------------
                    } // end if (reconfiguration(Mode, _adaptive, Precedence))
                    //--------------------------
                    return false;
                    //--------------------------
                } // end if (m_instance)
                //--------------------------
                m_instance = std::make_unique<std::any>(ThreadPool<Mode, Tick>(number_threads));
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
            constexpr bool reconfiguration(const ThreadMode& mode, bool adaptive_tick, const PrecedenceLevel& precedence) const {
                //--------------------------
                // Higher precedence overrides lower precedence
                if (precedence > m_current_precedence) {
                    return true;
                } // end if (precedence > m_current_precedence)
                //--------------------------
                // Priority mode overrides standard mode if precedence is the same
                if (precedence == m_current_precedence and mode == ThreadMode::PRIORITY and m_current_mode == ThreadMode::STANDARD) {
                    return true;
                } // end if (precedence == m_current_precedence and mode == ThreadMode::PRIORITY and m_current_mode == ThreadMode::STANDARD)
                //--------------------------
                // Non-adaptive tick overrides adaptive tick if precedence and mode are the same
                if (precedence == m_current_precedence and mode == m_current_mode and !adaptive_tick and m_adaptive_tick) {
                    return true;
                } // end if (precedence == m_current_precedence and mode == m_current_mode and !adaptive_tick and m_adaptive_tick)
                //--------------------------
                return false;
                //--------------------------
            } // end reconfiguration
            //--------------------------
            constexpr void update_configuration(const ThreadMode& mode, bool adaptive_tick, const PrecedenceLevel& precedence) {
                m_current_mode          = mode;
                m_adaptive_tick         = adaptive_tick;
                m_current_precedence    = precedence;
            } // end ThreadPoolManager::update_configuration
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            // Constructor using initializer list for efficiency
            ThreadPoolManager(void);
            ~ThreadPoolManager(void) = default;
            //--------------------------
            std::unique_ptr<std::any> m_instance;
            ThreadMode m_current_mode;
            bool m_adaptive_tick;
            PrecedenceLevel m_current_precedence;
        //--------------------------------------------------------------
    }; // end class ThreadPoolManager
    //--------------------------------------------------------------
} // end namespace ThreadPool
//--------------------------------------------------------------