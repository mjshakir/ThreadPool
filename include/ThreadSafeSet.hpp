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
            void add(const std::shared_ptr<std::jthread>& thread) {
                std::lock_guard<std::mutex> lock(mutex);
                set.insert(thread);
            }
            //--------------------------
            void remove(const std::shared_ptr<std::jthread>& thread) {
                std::lock_guard<std::mutex> lock(mutex);
                set.erase(thread);
            }
            //--------------------------
            std::optional<std::shared_ptr<std::jthread>> getOne() {
                std::lock_guard<std::mutex> lock(mutex);
                if (!set.empty()) {
                    auto it = set.begin();
                    std::shared_ptr<std::jthread> thread = *it;
                    set.erase(it);
                    return thread;
                }
                return std::nullopt;
            }
            //--------------------------
            bool empty() const {
                std::lock_guard<std::mutex> lock(mutex);
                return set.empty();
            }
            //--------------------------------------------------------------
        private:
            std::unordered_set<std::shared_ptr<std::jthread>> set;
            mutable std::mutex mutex;
        //--------------------------------------------------------------
    }; // end class ThreadSafeSet
    //--------------------------------------------------------------
}//end namespace ThreadPool