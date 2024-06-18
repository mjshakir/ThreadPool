//--------------------------------------------------------------
// Main Header 
//--------------------------------------------------------------
#include "ThreadTask.hpp"
//--------------------------------------------------------------
// Standard library
//--------------------------------------------------------------
#include <limits>
//--------------------------------------------------------------
// public
//--------------------------
ThreadPool::ThreadTask::ThreadTask(ThreadTask&& other) noexcept :   m_priority(other.m_priority),
                                                                    m_retries(other.m_retries),
                                                                    m_state(other.m_state) {
          
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_function = std::move(other.m_function);
        m_promise = std::move(other.m_promise);
    }
}// end ThreadPool::ThreadTask::ThreadTask(ThreadTask&&) noexcept     
//--------------------------------------------------------------
ThreadPool::ThreadTask& ThreadPool::ThreadTask::operator=(ThreadTask&& other) noexcept  {
    //--------------------------
    if (this != &other) {
        //--------------------------
        std::scoped_lock lock(m_mutex, other.m_mutex); // Lock both mutexes
        //--------------------------
        m_function  = std::move(other.m_function);
        m_priority  = other.m_priority;
        m_retries   = other.m_retries;
        m_state     = other.m_state;
        m_promise   = std::move(other.m_promise);
        //--------------------------
    }// end if (this != &other)
    //--------------------------
    return *this;
    //--------------------------
}// end ThreadPool::ThreadTask& ThreadPool::ThreadTask::operator=(ThreadTask&& other) noexcept
//--------------------------------------------------------------
bool ThreadPool::ThreadTask::operator==(const ThreadTask& other) const{
    std::scoped_lock lock(m_mutex, other.m_mutex); // Lock both mutexes
    return this == &other;
}// end bool ThreadPool::ThreadTask::operator==(const ThreadTask& other) 
//--------------------------------------------------------------
bool ThreadPool::ThreadTask::operator<(const ThreadTask& other) const{
    return ComparatorLess{}(*this, other);
}// end bool ThreadPool::ThreadTask::operator<(const ThreadTask& other) const
//--------------------------------------------------------------
bool ThreadPool::ThreadTask::operator>(const ThreadTask& other) const{
    return ComparatorMore{}(*this, other);
}// end bool ThreadPool::ThreadTask::operator<(const ThreadTask& other) const
//--------------------------------------------------------------
void ThreadPool::ThreadTask::execute(void){
    //--------------------------
    execute_local();
    //--------------------------
}// end void ThreadPool::ThreadTask::execute(void)
//--------------------------------------------------------------
bool ThreadPool::ThreadTask::try_execute(void){
    //--------------------------
    return try_execute_local();
    //--------------------------
}// end bool ThreadPool::ThreadTask::try_execute(void)
//--------------------------------------------------------------
std::future<std::any> ThreadPool::ThreadTask::get_future(void) {
    //--------------------------
    return get_future_local<decltype(m_function)>();
    //--------------------------
}// end std::any ThreadPool::ThreadTask::get_result(void) const
//--------------------------------------------------------------
bool ThreadPool::ThreadTask::done(void) const {
    //--------------------------
    return is_done();
    //--------------------------
}// end bool ThreadPool::ThreadTask::done(void) const
//--------------------------------------------------------------
uint8_t ThreadPool::ThreadTask::get_retries(void) const {
    //--------------------------
    return get_retries_local();
    //--------------------------
}// end uint8_t ThreadPool::ThreadTask::get_retries(void) const
//--------------------------
uint16_t ThreadPool::ThreadTask::get_priority(void) const {
    //--------------------------
    return get_priority_local();
    //--------------------------
}// end uint8_t ThreadPool::ThreadTask::get_priority(void) const
//--------------------------------------------------------------
void ThreadPool::ThreadTask::increase_retries(const uint8_t& amount){
    //--------------------------
    increase_retries_local(amount);
    //--------------------------
}// end void ThreadPool::ThreadTask::increase_retries(const uint8_t& amount)
//--------------------------------------------------------------
void ThreadPool::ThreadTask::decrease_retries(const uint8_t& amount){
    //--------------------------
    decrease_retries_local(amount);
    //--------------------------
}// end void ThreadPool::ThreadTask::decrease_retries(const uint8_t& amount)
//--------------------------------------------------------------
void ThreadPool::ThreadTask::increase_retries(void) {
    //--------------------------
    increase_retries_local(1);
    //--------------------------
}// end void ThreadPool::ThreadTask::increase_retries(void)
//--------------------------------------------------------------
void ThreadPool::ThreadTask::decrease_retries(void) {
    //--------------------------
    decrease_retries_local(1);
    //--------------------------
}// end void ThreadPool::ThreadTask::decrease_retries(void)
//--------------------------------------------------------------
void ThreadPool::ThreadTask::increase_priority(const uint16_t& amount){
    //--------------------------
    increase_priority_local(amount);
    //--------------------------
}// end void ThreadPool::ThreadTask::increase_priority(const uint8_t& amount)
//--------------------------------------------------------------
void ThreadPool::ThreadTask::decrease_priority(const uint16_t& amount){
    //--------------------------
    decrease_priority_local(amount);
    //--------------------------
}// end void ThreadPool::ThreadTask::decrease_priority(const uint8_t& amount)
//--------------------------------------------------------------
void ThreadPool::ThreadTask::increase_priority(void) {
    increase_priority_local(1);
} // end void ThreadPool::ThreadTask::increase_priority(void)
//--------------------------------------------------------------
void ThreadPool::ThreadTask::decrease_priority(void) {
    decrease_priority_local(1);
} // end void ThreadPool::ThreadTask::decrease_priority(void)
//--------------------------------------------------------------
uint8_t ThreadPool::ThreadTask::get_status(void) const {
    //--------------------------
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<uint8_t>(m_state);
    //--------------------------
}// end uint8_t ThreadPool::ThreadTask::get_status(void) const
//--------------------------------------------------------------
// protected
//--------------------------
void ThreadPool::ThreadTask::execute_local(void){
    //--------------------------
    do {
        //--------------------------
        if (try_execute()) {
            break;
        }// end if (try_execute())
        //--------------------------
        decrease_retries();
        //--------------------------
    } while (get_retries_local() > 0);
    //--------------------------
}// end void ThreadPool::ThreadTask::execute_local(void)
//--------------------------------------------------------------
bool ThreadPool::ThreadTask::try_execute_local(void){
    //--------------------------
    try {
         if constexpr (is_void_function<decltype(m_function)>()) {
            m_function();
        } else {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_promise.set_value(m_function());
        }
        //--------------------------
    } // end try
    catch (...) {
        //--------------------------
        return false;
        //--------------------------
    }// end catch (...)
    //--------------------------
    std::lock_guard<std::mutex> lock(m_mutex);
    //--------------------------
    m_state = ThreadPool::ThreadTask::TaskState::COMPLETED;
    //--------------------------
    return true;
    //--------------------------
}// end bool ThreadPool::ThreadTask::try_execute_local(void)
//--------------------------------------------------------------
bool ThreadPool::ThreadTask::is_done(void) const{
    //--------------------------
    std::lock_guard<std::mutex> lock(m_mutex);
    //--------------------------
    if (ThreadPool::ThreadTask::is_void_function<decltype(m_function)>() and (m_state == ThreadPool::ThreadTask::TaskState::COMPLETED)) {
        //--------------------------
        return true;
        //--------------------------
    }// end if (m_state == ThreadPool::ThreadTask::TaskState::COMPLETED)
    //--------------------------
    return m_state == ThreadPool::ThreadTask::TaskState::RETRIEVED;
    //--------------------------
}// end bool ThreadPool::ThreadTask::is_done(void) const
//--------------------------------------------------------------
uint8_t ThreadPool::ThreadTask::get_retries_local(void) const {
     //--------------------------
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_retries;
    //--------------------------
}// end uint8_t ThreadPool::ThreadTask::get_retries_local(void) const
//--------------------------------------------------------------
uint16_t ThreadPool::ThreadTask::get_priority_local(void) const {
     //--------------------------
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_priority;
    //--------------------------
}// end uint8_t ThreadPool::ThreadTask::get_priority_local(void) const
//--------------------------------------------------------------
void ThreadPool::ThreadTask::increase_retries_local(const uint8_t& amount) {
    //--------------------------
    std::lock_guard<std::mutex> lock(m_mutex);
    //--------------------------
    uint16_t _results = m_retries + amount;
    //--------------------------
    m_retries = (_results > std::numeric_limits<uint8_t>::max()) ? std::numeric_limits<uint8_t>::max() : _results;
    //--------------------------
}// end void ThreadPool::ThreadTask::increase_retries_local(const uint8_t& amount) const
//--------------------------------------------------------------
void ThreadPool::ThreadTask::decrease_retries_local(const uint8_t& amount) {
    //--------------------------
    std::lock_guard<std::mutex> lock(m_mutex);
    //--------------------------
    uint8_t _results = m_retries - amount;
    //--------------------------
    m_retries = std::clamp(_results, std::numeric_limits<uint8_t>::min(), std::numeric_limits<uint8_t>::max());
    //--------------------------
}// end void ThreadPool::ThreadTask::decrease_retries_local(const uint8_t& amount) const
//--------------------------------------------------------------
void ThreadPool::ThreadTask::increase_priority_local(const uint16_t& amount) {
    //--------------------------
    std::lock_guard<std::mutex> lock(m_mutex);
    //--------------------------
    uint16_t _results = m_priority + amount;
    //--------------------------
    m_priority = (_results > std::numeric_limits<uint16_t>::max()) ? std::numeric_limits<uint16_t>::max() : _results;
    //--------------------------
}// end void ThreadPool::ThreadTask::increase_priority_local(const uint16_t& amount) const
//--------------------------------------------------------------
void ThreadPool::ThreadTask::decrease_priority_local(const uint16_t& amount) {
    //--------------------------
    std::lock_guard<std::mutex> lock(m_mutex);
    //--------------------------
    uint16_t _results = m_priority - amount;
    //--------------------------
    m_priority = std::clamp(_results, std::numeric_limits<uint16_t>::min(), std::numeric_limits<uint16_t>::max());
    //--------------------------
}// end void ThreadPool::ThreadTask::decrease_priority_local(const uint16_t& amount) const
//--------------------------------------------------------------