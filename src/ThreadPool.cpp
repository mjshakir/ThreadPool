//--------------------------------------------------------------
// Main Header 
//--------------------------------------------------------------
#include "ThreadPool.hpp"
//--------------------------------------------------------------
// Standard library
//--------------------------------------------------------------      
#include <type_traits>
#include <chrono>
//--------------------------------------------------------------
// Definitions
//--------------------------
constexpr auto CHECK_INTERVAL = std::chrono::nanoseconds(100); 
constexpr size_t m_lowerThreshold = 1UL;
//--------------------------------------------------------------
ThreadPool::ThreadPool::ThreadPool(const size_t& numThreads) :  m_failedTasksCount(0),
                                                                m_retriedTasksCount(0),
                                                                m_completedTasksCount(0),
                                                                m_upperThreshold((static_cast<size_t>(std::thread::hardware_concurrency()) > 1) ? 
                                                                    static_cast<size_t>(std::thread::hardware_concurrency()) : m_lowerThreshold),
                                                                m_adjustmentThread([this](const std::stop_token& stoken){this->adjustmentThreadFunction(stoken);}){
    //--------------------------
    // auto _threads_number = std::max(std::min(m_upperThreshold, numThreads), m_lowerThreshold);
    auto _threads_number = std::clamp( numThreads, m_lowerThreshold, m_upperThreshold);
    create_task(_threads_number);
    //--------------------------
    m_tasks.reserve(numThreads);
    //--------------------------
}// end ThreadPool::ThreadPool::ThreadPool(const size_t& numThreads)
//--------------------------------------------------------------
ThreadPool::ThreadPool::~ThreadPool(void) {
    //--------------------------
    stop();
    //--------------------------
}// end ThreadPool::ThreadPool::~ThreadPool(void) 
//--------------------------------------------------------------
size_t ThreadPool::ThreadPool::threads_size(void) const{
    //--------------------------
    return thread_Workers_size();
    //--------------------------
}// end size_t ThreadPool::ThreadPool::threads_size() const
//--------------------------------------------------------------
size_t ThreadPool::ThreadPool::queued_size(void) const{
    //--------------------------
    return active_tasks_size();
    //--------------------------
}// end size_t ThreadPool::ThreadPool::queued_size() const
//--------------------------------------------------------------
std::tuple<size_t, size_t, size_t> ThreadPool::ThreadPool::status(void){
    //--------------------------
    return get_status();
    //--------------------------
}// end std::tuple<size_t, size_t, size_t> ThreadPool::ThreadPool::get_status(void)
//--------------------------------------------------------------
void ThreadPool::ThreadPool::status_disply(void){
    //--------------------------
    status_display_internal();
    //--------------------------
}// end void ThreadPool::ThreadPool::status_disply(void)
//--------------------------------------------------------------
void ThreadPool::ThreadPool::create_task(const size_t& numThreads){
    //--------------------------
    m_workers.reserve(m_workers.size() + numThreads);
    //--------------------------
    for (size_t i = 0; i < numThreads; ++i) {
        //--------------------------
        m_workers.emplace_back([this](std::stop_token stoken) {
            this->workerFunction(stoken);
        });
        //--------------------------
    }// end  for (size_t i = 0; i < numThreads; ++i)
    //--------------------------
}//end void ThreadPool::ThreadPool::create_task(const size_t& numThreads)
//--------------------------------------------------------------
void ThreadPool::ThreadPool::handleError(ThreadTask&& task, const char* error) {
    //--------------------------
    if (task.get_retries() > 0) {
        //--------------------------
        std::scoped_lock lock(m_mutex);
        //--------------------------;
        task.decrease_retries();
        m_tasks.push(std::move(task));
        //--------------------------
        ++m_retriedTasksCount;
        //--------------------------
    } else {
        //--------------------------
        ++m_failedTasksCount;
        std::cerr << "Error in task after multiple retries: " << error << std::endl;
        //--------------------------
    }// end if (task.get_retries() > 0)
    //--------------------------
}// end void ThreadPool::ThreadPool::handleError(ThreadTask&& task, const char* error)
//--------------------------------------------------------------
void ThreadPool::ThreadPool::workerFunction(const std::stop_token& stoken) {
    //--------------------------
    while (!stoken.stop_requested()) {
        //--------------------------
        ThreadTask task;
        //--------------------------
        {// being Append tasks 
            //--------------------------
            std::unique_lock lock(m_mutex);
            //--------------------------
            m_taskAvailableCondition.wait(lock, [this, &stoken] {return stoken.stop_requested() or !m_tasks.empty();});
            //--------------------------
            if (stoken.stop_requested() or m_tasks.empty()) {
                //--------------------------
                m_allStoppedCondition.notify_one();
                //--------------------------
                return;
                //--------------------------
            }// end if (stoken.stop_requested() and m_tasks.empty())
            //--------------------------
            task = std::move(m_tasks.pop_top().value());
            //--------------------------
        }// end Append tasks
        //--------------------------
        try {
            //--------------------------
            if(task.try_execute()){
                ++m_completedTasksCount;
            }// end if(_success)
            //--------------------------
        } // end try
        catch (const std::exception& e) {
            //--------------------------
            handleError(std::move(task), e.what());
            //--------------------------
        } // end catch (const std::exception& e)
        catch (...) {
            //--------------------------
            handleError(std::move(task), "Unknown error");
            //--------------------------
        }// end catch (...)
        //--------------------------
    }// end while (!stoken.stop_requested())
    //--------------------------
}// end void ThreadPool::ThreadPool::workerFunction(void)
//--------------------------------------------------------------
void ThreadPool::ThreadPool::stop(void){
    //--------------------------
    {
        //--------------------------
        std::unique_lock lock(m_mutex);
        //--------------------------
        m_allStoppedCondition.wait(lock, [this] { return m_tasks.empty(); }); 
        //--------------------------
        m_adjustmentThread.request_stop();
        //--------------------------
        for (auto &worker : m_workers) {
            worker.request_stop(); // request each worker to stop
        }// end for (auto &worker : m_workers)
    }
    //--------------------------
    m_taskAvailableCondition.notify_all();
    //--------------------------
}//end void ThreadPool::ThreadPool::stop(void)
//--------------------------------------------------------------
void ThreadPool::ThreadPool::adjustWorkers(void) {
    //--------------------------
    m_tasks.remove();
    //--------------------------
    const auto taskCount = active_tasks_size(), workerCount = thread_Workers_size();
    //--------------------------
    {
        //--------------------------
        std::unique_lock lock(m_mutex);
        //--------------------------
        // if (workerCount > taskCount) {
            //--------------------------
            // for (size_t i = 0; i < workerCount - taskCount and !m_workers.empty() and !m_adjustmentThread.get_stop_token().stop_requested(); ++i) {
                // m_workers.back().request_stop(); // Request each worker to stop
                // m_allStoppedCondition.wait(lock, [this] { return m_workers.back().get_stop_token().stop_requested();});
                //--------------------------
                // if(m_workers.back().joinable()){
                    // m_workers.back().join();
                    // m_workers.pop_back(); // Safely remove the stopped worker
                // }// end if(m_workers.back().joinable())
            // }
            //--------------------------
        // }// end if (workerCount > taskCount)
        //--------------------------
        if (taskCount > workerCount and workerCount < m_upperThreshold) {
            create_task(std::min(taskCount - workerCount, m_upperThreshold - workerCount));
        }
        //--------------------------
    }
    m_allStoppedCondition.notify_one();
    //--------------------------
}// end void ThreadPool::ThreadPool::adjustWorkers(void)
//--------------------------------------------------------------
void ThreadPool::ThreadPool::adjustmentThreadFunction(const std::stop_token& stoken) {
    //--------------------------
    while (!stoken.stop_requested()) {
        //--------------------------
        std::this_thread::sleep_for(CHECK_INTERVAL);
        //--------------------------
        adjustWorkers();
        //--------------------------
    }// end while (!stoken.stop_requested())
    //--------------------------
}// end void ThreadPool::ThreadPool::adjustmentThreadFunction(const std::stop_token& stoken)
//--------------------------------------------------------------
void ThreadPool::ThreadPool::push_task(ThreadTask&& task) {
    //--------------------------
    m_tasks.push(std::move(task));
    //--------------------------
    m_taskAvailableCondition.notify_one();
    //--------------------------
}// end void ThreadPool::ThreadPool::push_task(Task&& task)
//--------------------------------------------------------------
size_t ThreadPool::ThreadPool::thread_Workers_size(void) const{
    //--------------------------
    std::unique_lock lock(m_mutex);
    //--------------------------
    return m_workers.size();
    //--------------------------
}// end size_t ThreadPool::ThreadPool::thread_Workers_size(void) const
//--------------------------------------------------------------
size_t ThreadPool::ThreadPool::active_tasks_size(void) const{
    //--------------------------
    return m_tasks.size();
    //--------------------------
}// end size_t ThreadPool::ThreadPool::active_tasks_size(void) const
//--------------------------------------------------------------
std::tuple<size_t, size_t, size_t> ThreadPool::ThreadPool::get_status(void){
    //--------------------------
    return {m_failedTasksCount.load(), m_retriedTasksCount.load(), m_completedTasksCount.load()};
    //--------------------------
}// end std::tuple<size_t, size_t, size_t> ThreadPool::ThreadPool::get_status(void)
//--------------------------------------------------------------
void ThreadPool::ThreadPool::status_display_internal(void){
    //--------------------------
    auto [failedTasksCount, retriedTasksCount, completedTasksCount] = get_status();
    //--------------------------
    std::cout   << "Failed Tasks:    " << failedTasksCount  << "\n"
                << "Retried Tasks:   " << retriedTasksCount << "\n"
                << "Completed Tasks: " << completedTasksCount << std::endl;
    //--------------------------
}// end void ThreadPool::ThreadPool::status_display_internal(void)
//--------------------------------------------------------------