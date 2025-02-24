#pragma once

//--------------------------------------------------------------
// Standard library
//--------------------------------------------------------------
#include <cstdbool>
#include <string_view>
//--------------------------------------------------------------
/** @namespace ThreadPool
 * @brief A namespace containing the ThreadPool class.
 */
namespace ThreadPool {
    //--------------------------------------------------------------
    /**
     * @enum ThreadMode
     * @brief Enum class to specify threading mode for ThreadPool.
     *
     * This enum class is used to define the threading mode for the ThreadPool class. It supports two modes:
     * STANDARD and PRIORITY. STANDARD mode is used for regular operation, while PRIORITY mode allows for
     * priority-based task execution within the ThreadPool.
     *
     * @param ThreadMode::STANDARD
     * STANDARD mode indicates that the ThreadPool will process tasks in a first-come, first-served manner,
     * without considering task priority. This is the default mode.
     *
     * @param ThreadMode::PRIORITY
     * PRIORITY mode indicates that the ThreadPool will process tasks based on their priority, allowing
     * higher priority tasks to be executed before lower priority ones.
     */
    enum class ThreadMode : bool {
        STANDARD = false,
        PRIORITY = true
    }; // end enum class ThreadMode
    //--------------------------------------------------------------
    /**
     * @brief Converts the ThreadMode enum to a string representation.
     *
     * @details This function converts the ThreadMode enum to a string representation for easy debugging and logging.
     *
     * @param mode The ThreadMode enum value to convert.
     * 
     * @return std::string_view A string representation of the ThreadMode enum value.
     * 
     * @example
     * 
     * @code
     * 
     * #include <iostream>
     * #include "ThreadMode.hpp"
     * 
     * void main() {
     *     constexpr ThreadPool::ThreadMode mode = ThreadPool::ThreadMode::PRIORITY;
     *     constexpr std::string_view mode_str = ThreadPool::ThreadMode_name(mode);
     *     
     *     std::cout << "Thread mode: " << mode_str << std::endl; // Outputs: "PRIORITY"
     *     return 0;
     * }
     * 
     * @endcode
     */
    constexpr std::string_view ThreadMode_name(const ThreadMode& mode) {
        //--------------------------------------------------------------
        switch (mode) {
            //--------------------------
            case ThreadMode::STANDARD:
                return "STANDARD";
            //--------------------------
            case ThreadMode::PRIORITY:
                return "PRIORITY";
            //--------------------------
            default:
                return "UNKNOWN";
            //--------------------------
        }// end switch (mode)
        //--------------------------------------------------------------
    }// end constexpr std::string_view to_string(ThreadMode mode)
    //--------------------------------------------------------------
} // end namespace ThreadPool
//--------------------------------------------------------------