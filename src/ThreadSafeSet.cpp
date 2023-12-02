//--------------------------------------------------------------
// Main Header 
//--------------------------------------------------------------
#include "ThreadSafeSet.hpp"
//--------------------------------------------------------------
ThreadPool::ThreadSafeSet::ThreadSafeSet(const size_t& size){
    //--------------------------
    m_set.reserve(size);
    //--------------------------
}// end ThreadPool::ThreadSafeSet::ThreadSafeSet(const size_t& size)
//--------------------------------------------------------------
void ThreadPool::ThreadSafeSet::add(const std::shared_ptr<std::jthread>& thread){
    //--------------------------
    add_thread(thread);
    //--------------------------
}// end void ThreadPool::ThreadSafeSet::add(const std::shared_ptr<std::jthread>& thread)
//--------------------------------------------------------------
void ThreadPool::ThreadSafeSet::remove(const std::shared_ptr<std::jthread>& thread){
    //--------------------------
    remove_thread(thread);
    //--------------------------
}// end void ThreadPool::ThreadSafeSet::remove(const std::shared_ptr<std::jthread>& thread)
//--------------------------------------------------------------
bool ThreadPool::ThreadSafeSet::empty(void) const{
    //--------------------------
    return empty_set();
    //--------------------------
}// end void ThreadPool::ThreadSafeSet::remove(const std::shared_ptr<std::jthread>& thread)
//--------------------------------------------------------------
void ThreadPool::ThreadSafeSet::reserve(const size_t& size){
    //--------------------------
    reserve_size(size);
    //--------------------------
}// end void ThreadPool::ThreadSafeSet::reserve(const size_t& size)
//--------------------------------------------------------------
std::optional<std::shared_ptr<std::jthread>> ThreadPool::ThreadSafeSet::get_thread(void){
    //--------------------------
    return get_thread_local();
    //--------------------------
}// end void ThreadPool::ThreadSafeSet::remove(const std::shared_ptr<std::jthread>& thread)
//--------------------------------------------------------------
void ThreadPool::ThreadSafeSet::add_thread(const std::shared_ptr<std::jthread>& thread){
    //--------------------------
    std::lock_guard<std::mutex> lock(m_mutex);
    m_set.insert(thread);
    //--------------------------
}// end void ThreadPool::ThreadSafeSet add_thread(const std::shared_ptr<std::jthread>& thread)
//--------------------------------------------------------------
void ThreadPool::ThreadSafeSet::remove_thread(const std::shared_ptr<std::jthread>& thread){
    //--------------------------
    std::lock_guard<std::mutex> lock(m_mutex);
    m_set.erase(thread);
    //--------------------------
}// end void ThreadPool::ThreadSafeSet::remove_thread(const std::shared_ptr<std::jthread>& thread)
//--------------------------------------------------------------
bool ThreadPool::ThreadSafeSet::empty_set(void) const{
    //--------------------------
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_set.empty();
    //--------------------------
}// end bool ThreadPool::ThreadSafeSet::empty_set(void) const
//--------------------------------------------------------------
void ThreadPool::ThreadSafeSet::reserve_size(const size_t& size){
    //--------------------------
    m_set.reserve(size);
    //--------------------------
}// end void ThreadPool::ThreadSafeSet::reserve_size(const size_t& size)
//--------------------------------------------------------------
std::optional<std::shared_ptr<std::jthread>> ThreadPool::ThreadSafeSet::get_thread_local(void){
    //--------------------------
    std::lock_guard<std::mutex> lock(m_mutex);
    //--------------------------
    if (!m_set.empty()) {
        //--------------------------
        auto it = m_set.begin();
        std::shared_ptr<std::jthread> thread = *it;
        m_set.erase(it);
        //--------------------------
        return thread;
        //--------------------------
    }// end if (!m_set.empty()) 
    //--------------------------
    return std::nullopt;
    //--------------------------
}// end std::optional<std::shared_ptr<std::jthread>> ThreadPool::ThreadSafeSet::get_thread(void)
//--------------------------------------------------------------