//--------------------------------------------------------------
// Main Header 
//--------------------------------------------------------------
#include "ThreadPoolManager.hpp"
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
constexpr bool ThreadPool::ThreadPoolManager::initialized(void) const {
    return m_instance != nullptr;
} // end ThreadPool::ThreadPoolManager::initialized(void)
//--------------------------------------------------------------
ThreadPool::ThreadPool<>& ThreadPool::ThreadPoolManager::get_thread_pool(void) const {
    //--------------------------
    if (!m_instance) {
        static ThreadPool<> default_pool;
        return default_pool;  // Fallback: Provide a default pool.
    } // end if (!m_instance)
    //--------------------------
    return *m_instance;
    //--------------------------
} // end ThreadPool::ThreadPoolManager::get_thread_pool(void)
//--------------------------------------------------------------
constexpr bool ThreadPool::ThreadPoolManager::should_override_configuration(ThreadMode mode, bool isAdaptiveTick, PrecedenceLevel precedence) const {
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
    if (precedence == m_current_precedence && mode == m_current_mode && !isAdaptiveTick && m_adaptive_tick) {
        return true;
    } // end if (precedence == m_current_precedence && mode == m_current_mode && !isAdaptiveTick && m_adaptive_tick)
    //--------------------------
    return false;
    //--------------------------
} // end ThreadPoolManager::should_override_configuration
//--------------------------------------------------------------
constexpr void ThreadPool::ThreadPoolManager::update_configuration(ThreadMode mode, bool isAdaptiveTick, PrecedenceLevel precedence) {
    m_current_mode          = mode;
    m_adaptive_tick         = isAdaptiveTick;
    m_current_precedence    = precedence;
} // end ThreadPoolManager::update_configuration
//--------------------------------------------------------------