//--------------------------------------------------------------
// Main Header 
//--------------------------------------------------------------
#include "ThreadPoolManager.hpp"
//--------------------------------------------------------------
ThreadPool::ThreadPoolManager::ThreadPoolManager(const size_t& number_threads) : m_thread_pool(number_threads) {
    //--------------------------------------------------------------
}//end ThreadPool::ThreadPoolManager::ThreadPoolManager(const size_t& number_threads)
//--------------------------------------------------------------
ThreadPool::ThreadPoolManager& ThreadPool::ThreadPoolManager::get_instance(const size_t& number_threads){
    //--------------------------------------------------------------
    static ThreadPoolManager instance(number_threads);
    return instance;
    //--------------------------------------------------------------
}//end ThreadPool::ThreadPoolManager& ThreadPool::ThreadPoolManager::get_instance(void)
//--------------------------------------------------------------