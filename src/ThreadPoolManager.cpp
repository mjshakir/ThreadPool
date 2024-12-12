//--------------------------------------------------------------
// Main Header 
//--------------------------------------------------------------
#include "ThreadPoolManager.hpp"
//--------------------------------------------------------------
// Standard library
//--------------------------------------------------------------
#include <thread>
//--------------------------------------------------------------
ThreadPool::ThreadPoolManager::ThreadPoolManager(void) :    m_instance(nullptr),
                                                            m_current_mode(ThreadMode::STANDARD),
                                                            m_adaptive_tick(false),
                                                            m_current_precedence(PrecedenceLevel::LOW) {
    //--------------------------
}// end ThreadPool::ThreadPoolManager::ThreadPoolManager(void)
//--------------------------------------------------------------
ThreadPool::ThreadPoolManager& ThreadPool::ThreadPoolManager::get_instance(void) {
    static ThreadPoolManager instance;
    return instance;
} // end ThreadPool::ThreadPoolManager::get_instance(void)
//--------------------------------------------------------------
bool ThreadPool::ThreadPoolManager::initialized(void) const {
    return static_cast<bool>(m_instance);
} // end ThreadPool::ThreadPoolManager::initialized(void)
//--------------------------------------------------------------
ThreadPool::ThreadPool<>& ThreadPool::ThreadPoolManager::get_thread_pool(void) const {
    //--------------------------
    if (!m_instance) {
        static ThreadPool<> default_pool(static_cast<size_t>(std::thread::hardware_concurrency()));
        return default_pool;  // Fallback: Provide a default pool.
    } // end if (!m_instance)
    //--------------------------
    return *m_instance;
    //--------------------------
} // end ThreadPool::ThreadPoolManager::get_thread_pool(void)
//--------------------------------------------------------------
constexpr bool ThreadPool::ThreadPoolManager::should_override_configuration(const ThreadMode& mode, bool adaptive_tick, const PrecedenceLevel& precedence) const {
    //--------------------------
    // Higher precedence overrides lower precedence
    if (precedence > m_current_precedence) {
        return true;
    } // end if (precedence > m_current_precedence)
    //--------------------------
    // Priority mode overrides standard mode if precedence is the same
    if (precedence == m_current_precedence && mode == ThreadMode::PRIORITY && m_current_mode == ThreadMode::STANDARD) {
        return true;
    } // end if (precedence == m_current_precedence && mode == ThreadMode::PRIORITY && m_current_mode == ThreadMode::STANDARD)
    //--------------------------
    // Non-adaptive tick overrides adaptive tick if precedence and mode are the same
    if (precedence == m_current_precedence && mode == m_current_mode && !adaptive_tick && m_adaptive_tick) {
        return true;
    } // end if (precedence == m_current_precedence && mode == m_current_mode && !adaptive_tick && m_adaptive_tick)
    //--------------------------
    return false;
    //--------------------------
} // end ThreadPoolManager::should_override_configuration
//--------------------------------------------------------------
constexpr void ThreadPool::ThreadPoolManager::update_configuration(const ThreadMode& mode, bool adaptive_tick, const PrecedenceLevel& precedence) {
    m_current_mode          = mode;
    m_adaptive_tick         = adaptive_tick;
    m_current_precedence    = precedence;
} // end ThreadPoolManager::update_configuration
//--------------------------------------------------------------