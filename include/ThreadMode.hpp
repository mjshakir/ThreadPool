#pragma once

//--------------------------------------------------------------
// Standard library
//--------------------------------------------------------------
#include <iostream>
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
} // end namespace ThreadPool
//--------------------------------------------------------------