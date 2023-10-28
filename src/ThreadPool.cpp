//--------------------------------------------------------------
// Main Header 
//--------------------------------------------------------------
#include "ThreadPool.hpp"
//--------------------------------------------------------------
// Standard library
//--------------------------------------------------------------      
// #include <type_traits>
// #include <chrono>
//--------------------------------------------------------------
// Definitions
//--------------------------
// constexpr auto CHECK_INTERVAL = std::chrono::nanoseconds(100); 
// constexpr size_t m_lowerThreshold = 1UL;
//--------------------------------------------------------------
// template <bool use_priority_queue>
// ThreadPool::ThreadPool<use_priority_queue>::ThreadPool(const size_t& numThreads) 
//                             :   m_upperThreshold((static_cast<size_t>(std::thread::hardware_concurrency()) > 1) ? 
//                                     static_cast<size_t>(std::thread::hardware_concurrency()) : m_lowerThreshold),
//                                 m_adjustmentThread([this](const std::stop_token& stoken){this->adjustmentThreadFunction(stoken);}){
//     //--------------------------
//     // auto _threads_number = std::max(std::min(m_upperThreshold, numThreads), m_lowerThreshold);
//     auto _threads_number = std::clamp( numThreads, m_lowerThreshold, m_upperThreshold);
//     create_task(_threads_number);
//     //--------------------------
//     m_tasks.reserve(numThreads);
//     //--------------------------
// }// end ThreadPool::ThreadPool::ThreadPool(const size_t& numThreads)
//--------------------------------------------------------------
// template <bool use_priority_queue>
// ThreadPool::ThreadPool<use_priority_queue>::~ThreadPool(void) {
//     //--------------------------
//     stop();
//     //--------------------------
// }// end ThreadPool::ThreadPool::~ThreadPool(void) 
//--------------------------------------------------------------
// template <bool use_priority_queue>
// size_t ThreadPool::ThreadPool<use_priority_queue>::threads_size(void) const{
//     //--------------------------
//     return thread_Workers_size();
//     //--------------------------
// }// end size_t ThreadPool::ThreadPool::threads_size() const
//--------------------------------------------------------------
// template <bool use_priority_queue>
// size_t ThreadPool::ThreadPool<use_priority_queue>::queued_size(void) const{
//     //--------------------------
//     return active_tasks_size();
//     //--------------------------
// }// end size_t ThreadPool::ThreadPool::queued_size() const
//--------------------------------------------------------------
// template <bool use_priority_queue>
// void ThreadPool::ThreadPool<use_priority_queue>::create_task(const size_t& numThreads){
//     //--------------------------
//     m_workers.reserve(m_workers.size() + numThreads);
//     //--------------------------
//     for (size_t i = 0; i < numThreads; ++i) {
//         //--------------------------
//         m_workers.emplace_back([this](std::stop_token stoken) {
//             this->workerFunction(stoken);
//         });
//         //--------------------------
//     }// end  for (size_t i = 0; i < numThreads; ++i)
//     //--------------------------
// }//end void ThreadPool::ThreadPool::create_task(const size_t& numThreads)
//--------------------------------------------------------------
// template <bool use_priority_queue>
// void ThreadPool::ThreadPool<use_priority_queue>::handleError(ThreadTask&& task, const char* error) {
//     //--------------------------
//     if (task.get_retries() > 0) {
//         //--------------------------
//         std::scoped_lock lock(m_mutex);
//         //--------------------------;
//         task.decrease_retries();
//         m_tasks.push(std::move(task));
//         //--------------------------
//         ++m_retriedTasksCount;
//         //--------------------------
//     } else {
//         //--------------------------
//         ++m_failedTasksCount;
//         std::cerr << "Error in task after multiple retries: " << error << std::endl;
//         //--------------------------
//     }// end if (task.get_retries() > 0)
//     //--------------------------
// }// end void ThreadPool::ThreadPool::handleError(ThreadTask&& task, const char* error)
//--------------------------------------------------------------
// template <bool use_priority_queue>
// void ThreadPool::ThreadPool<use_priority_queue>::handleError(const char* error) {
//     //--------------------------
//     std::cerr << "Error in task: " << error << std::endl;
//     //--------------------------
// }// end void ThreadPool::ThreadPoolDeque::handleError(ThreadTask&& task, const char* error)
//--------------------------------------------------------------
// template <bool use_priority_queue>
// void ThreadPool::ThreadPool<use_priority_queue>::workerFunction(const std::stop_token& stoken) {
//     //--------------------------
//     while (!stoken.stop_requested()) {
//         //--------------------------
//         using TaskType = std::conditional_t<use_priority_queue, ThreadTask, std::function<void()>>;
//         TaskType task;
//         //--------------------------
//         {// being Append tasks 
//             //--------------------------
//             std::unique_lock lock(m_mutex);
//             //--------------------------
//             m_taskAvailableCondition.wait(lock, [this, &stoken] {return stoken.stop_requested() or !m_tasks.empty();});
//             //--------------------------
//             if (stoken.stop_requested() or m_tasks.empty()) {
//                 //--------------------------
//                 m_allStoppedCondition.notify_one();
//                 //--------------------------
//                 return;
//                 //--------------------------
//             }// end if (stoken.stop_requested() and m_tasks.empty())
//             //--------------------------
//             if constexpr (use_priority_queue){
//                 task = std::move(m_tasks.pop_top().value());
//             } else{
//                 task = std::move(m_tasks.front());
//                 m_tasks.pop_front();
//             }// end if constexpr (use_priority_queue)
//             //--------------------------
//         }// end Append tasks
//         //--------------------------
//         try {
//             //--------------------------
//             if constexpr (use_priority_queue){
//                 static_cast<void>(task.try_execute());
//             } else{
//                 task();
//             }// end if constexpr (use_priority_queue)
//             //--------------------------
//         } // end try
//         catch (const std::exception& e) {
//             //--------------------------
//             if constexpr (use_priority_queue){
//                 handleError(std::move(task), e.what());
//             } else{
//                 handleError(e.what());
//             }// end if constexpr (use_priority_queue)
//             //--------------------------
//         } // end catch (const std::exception& e)
//         catch (...) {
//             //--------------------------
//             if constexpr (use_priority_queue){
//                 handleError(std::move(task), "Unknown error");
//             } else{
//                 handleError("Unknown error");
//             }// end if constexpr (use_priority_queue)
//             //--------------------------
//         }// end catch (...)
//         //--------------------------
//     }// end while (!stoken.stop_requested())
//     //--------------------------
// }// end void ThreadPool::ThreadPool::workerFunction(void)
//--------------------------------------------------------------
// template <bool use_priority_queue>
// void ThreadPool::ThreadPool<use_priority_queue>::stop(void){
//     //--------------------------
//     {
//         //--------------------------
//         std::unique_lock lock(m_mutex);
//         //--------------------------
//         m_allStoppedCondition.wait(lock, [this] { return m_tasks.empty(); }); 
//         //--------------------------
//         m_adjustmentThread.request_stop();
//         //--------------------------
//         for (auto &worker : m_workers) {
//             worker.request_stop(); // request each worker to stop
//         }// end for (auto &worker : m_workers)
//     }
//     //--------------------------
//     m_taskAvailableCondition.notify_all();
//     //--------------------------
// }//end void ThreadPool::ThreadPool::stop(void)
//--------------------------------------------------------------
// template <bool use_priority_queue>
// void ThreadPool::ThreadPool<use_priority_queue>::adjustWorkers(void) {
//     //--------------------------
//     if constexpr (use_priority_queue){
//         m_tasks.remove();
//     }// end if constexpr (use_priority_queue)
//     //--------------------------
//     const auto taskCount = active_tasks_size(), workerCount = thread_Workers_size();
//     //--------------------------
//     {
//         //--------------------------
//         std::unique_lock lock(m_mutex);
//         //--------------------------
//         // if (workerCount > taskCount) {
//             //--------------------------
//             // for (size_t i = 0; i < workerCount - taskCount and !m_workers.empty() and !m_adjustmentThread.get_stop_token().stop_requested(); ++i) {
//                 // m_workers.back().request_stop(); // Request each worker to stop
//                 // m_allStoppedCondition.wait(lock, [this] { return m_workers.back().get_stop_token().stop_requested();});
//                 //--------------------------
//                 // if(m_workers.back().joinable()){
//                     // m_workers.back().join();
//                     // m_workers.pop_back(); // Safely remove the stopped worker
//                 // }// end if(m_workers.back().joinable())
//             // }
//             //--------------------------
//         // }// end if (workerCount > taskCount)
//         //--------------------------
//         if (taskCount > workerCount and workerCount < m_upperThreshold) {
//             create_task(std::min(taskCount - workerCount, m_upperThreshold - workerCount));
//         }
//         //--------------------------
//     }
//     m_allStoppedCondition.notify_one();
//     //--------------------------
// }// end void ThreadPool::ThreadPool::adjustWorkers(void)
//--------------------------------------------------------------
// template <bool use_priority_queue>
// void ThreadPool::ThreadPool<use_priority_queue>::adjustmentThreadFunction(const std::stop_token& stoken) {
//     //--------------------------
//     while (!stoken.stop_requested()) {
//         //--------------------------
//         std::this_thread::sleep_for(CHECK_INTERVAL);
//         //--------------------------
//         adjustWorkers();
//         //--------------------------
//     }// end while (!stoken.stop_requested())
//     //--------------------------
// }// end void ThreadPool::ThreadPool::adjustmentThreadFunction(const std::stop_token& stoken)
//--------------------------------------------------------------
// template <bool use_priority_queue>
// void ThreadPool::ThreadPool<use_priority_queue>::push_task(ThreadTask&& task) {
//     //--------------------------
//     m_tasks.push(std::move(task));
//     //--------------------------
//     m_taskAvailableCondition.notify_one();
//     //--------------------------
// }// end void ThreadPool::ThreadPool::push_task(Task&& task)
//--------------------------------------------------------------
// template <bool use_priority_queue>
// size_t ThreadPool::ThreadPool<use_priority_queue>::thread_Workers_size(void) const{
//     //--------------------------
//     std::unique_lock lock(m_mutex);
//     //--------------------------
//     return m_workers.size();
//     //--------------------------
// }// end size_t ThreadPool::ThreadPool::thread_Workers_size(void) const
//--------------------------------------------------------------
// template <bool use_priority_queue>
// size_t ThreadPool::ThreadPool<use_priority_queue>::active_tasks_size(void) const{
//     //--------------------------
//     if constexpr (!use_priority_queue){
//         std::unique_lock lock(m_mutex);
//     }// end if constexpr (!use_priority_queue){
//     //--------------------------
//     return m_tasks.size();
//     //--------------------------
// }// end size_t ThreadPool::ThreadPool::active_tasks_size(void) const
//--------------------------------------------------------------