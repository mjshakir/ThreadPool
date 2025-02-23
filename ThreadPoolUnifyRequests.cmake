#------------------------------------------------------------------------------------------
# ThreadPoolUnifyRequests.cmake
#
# This file provides:
#
# 1. threadpool_request_config(MODE, TICK)
#    - Records a configuration request in the form: "<Caller>:<mode>,<tick>"
#      where <Caller> is automatically taken from ${PROJECT_NAME} (or "Unknown" if not defined).
#
# 2. _threadpool_unify_requests()
#    - Reads all accumulated requests, prints them, and computes the final
#      unified configuration:
#        - Final mode is "1" (PRIORITY) if any request has mode "1"; otherwise "0" (STANDARD).
#        - For tick values: if any request yields 0 (non-adaptive), then the final
#          tick is 0; otherwise, the maximum tick value is chosen.
#
# Parent projects can set a global variable (e.g. GLOBAL_THREADPOOL_REQUEST_LIST)
# before calling add_subdirectory() so that their requests are seen.
#------------------------------
# Global variables:
if(DEFINED GLOBAL_THREADPOOL_REQUEST_LIST)
    set(_THREADPOOL_REQUEST_LIST "${GLOBAL_THREADPOOL_REQUEST_LIST}" CACHE INTERNAL "Accumulated requests for ThreadPool" FORCE)
endif()
#------------------------------
# Default values for ThreadPool.
if(NOT DEFINED _THREADPOOL_REQUEST_LIST)
    set(_THREADPOOL_REQUEST_LIST "" CACHE INTERNAL "Accumulated requests for ThreadPool" FORCE)
endif()
#------------------------------
# Count of threadpool_request_config calls.
if(NOT DEFINED _THREADPOOL_REQUEST_COUNT)
    set(_THREADPOOL_REQUEST_COUNT 0 CACHE INTERNAL "Count of threadpool_request_config calls" FORCE)
endif()
#------------------------------
# Flag to check if unification is done.
if(NOT DEFINED _THREADPOOL_UNIFIED)
    set(_THREADPOOL_UNIFIED "OFF" CACHE INTERNAL "Flag to check if unification is done" FORCE)
endif()
#------------------------------------------------------------------------------------------
# Function: threadpool_request_config(MODE, TICK)
#
# Records a configuration request. The request is stored in the format:
#   <Caller>:<mode>,<tick>
# where <Caller> is automatically set to ${PROJECT_NAME} (or "Unknown" if not defined).
#------------------------------
function(threadpool_request_config mode tick)
    if(DEFINED PROJECT_NAME)
        set(_caller "${PROJECT_NAME}")
    else()
        set(_caller "Unknown")
    endif()
    set(_request "${_caller}:${mode},${tick}")
    if(DEFINED GLOBAL_THREADPOOL_REQUEST_LIST)
        set(GLOBAL_THREADPOOL_REQUEST_LIST "${GLOBAL_THREADPOOL_REQUEST_LIST};${_request}" CACHE INTERNAL "Global ThreadPool configuration requests" FORCE)
    else()
        set(GLOBAL_THREADPOOL_REQUEST_LIST "${_request}" CACHE INTERNAL "Global ThreadPool configuration requests" FORCE)
    endif()
    set(_prev "${_THREADPOOL_REQUEST_LIST}")
    set(_new  "${_prev};${_request}")
    set(_THREADPOOL_REQUEST_LIST "${_new}" CACHE INTERNAL "Accumulated requests for ThreadPool" FORCE)
    math(EXPR _new_count "${_THREADPOOL_REQUEST_COUNT} + 1")
    set(_THREADPOOL_REQUEST_COUNT "${_new_count}" CACHE INTERNAL "Count of threadpool_request_config calls" FORCE)
    message(STATUS "[${_caller}] ThreadPool request: Mode=${mode}, Tick=${tick}")
endfunction()
#------------------------------------------------------------------------------------------
# Function: _threadpool_unify_requests()
#
# Unifies the configuration requests and sets the cache variables:
#   THREADPOOL_MODE and THREADPOOL_ADOPTIVE_TICK.
#
# Each request should be in the format: "Caller:mode,tick".
# For tick values:
#   - If any request yields a tick of 0, then the final tick becomes 0.
#   - Otherwise, the highest (maximum) tick value is used.
#------------------------------
function(_threadpool_unify_requests)
    # Prevent re-unification if already done.
    if(_THREADPOOL_UNIFIED STREQUAL "ON")
        return()
    endif()
    
    # If there are no requests, use the defaults.
    if(_THREADPOOL_REQUEST_LIST STREQUAL "")
        set(_REQUESTS "${THREADPOOL_DEFAULT_MODE};${THREADPOOL_DEFAULT_ADOPTIVE_TICK}")
    else()
        # Here we assume that each request is already separated by a semicolon.
        set(_REQUESTS "${_THREADPOOL_REQUEST_LIST}")
    endif()
    
    separate_arguments(_REQUESTS)
    
    message(STATUS "=== ThreadPool Requests ===")
    foreach(req IN LISTS _REQUESTS)
        message(STATUS "  Request: ${req}")
    endforeach()
    message(STATUS "===========================")
    
    # Initialize unified values.
    set(_unified_mode "0")  # Default STANDARD
    set(_unified_tick "None")
    
    foreach(req IN LISTS _REQUESTS)
        # Expect each request in the form: "Caller:mode,tick"
        if(req STREQUAL "")
            continue()
        endif()
        string(FIND "${req}" ":" sep_pos)
        if(sep_pos EQUAL -1)
            if(NOT req STREQUAL "0")
                message(WARNING "Malformed request: ${req}")
            endif()
            continue()
        endif()
        # Compute offset = sep_pos + 1
        math(EXPR offset "${sep_pos} + 1")
        string(LENGTH "${req}" req_length)
        math(EXPR num_length "${req_length} - ${offset}")
        string(SUBSTRING "${req}" ${offset} ${num_length} numeric_part)
        string(REPLACE "," ";" parts "${numeric_part}")
        list(GET parts 0 mode_str)
        list(GET parts 1 tick_str)
        
        # Debug print:
        message(STATUS "[ThreadPool] Parsed request: Mode=${mode_str}, Tick=${tick_str}")
        
        # (A) Mode: if any request's mode is "1", then final mode becomes "1".
        if(mode_str STREQUAL "1")
            set(_unified_mode "1")
        endif()
        
        # (B) Process tick_str:
        # If tick_str is exactly "0", then that's a non-adaptive setting.
        if(tick_str STREQUAL "0")
            set(sanitized_val 0)
            set(_unified_tick 0)
            continue()
        endif()
        
        string(REGEX REPLACE "[^0-9\\-\\+\\.]" "" tick_clean "${tick_str}")
        set(sanitized_val "0")
        if(tick_clean MATCHES "^[+-]?[0-9]+(\\.[0-9]+)?$")
            math(EXPR parsed_float "0 + ${tick_clean}")
            set(floor_val "${parsed_float}")
            if(floor_val MATCHES "^-?[0-9]+\\.[0-9]+$")
                string(REGEX REPLACE "\\.[0-9]+$" "" floor_val "${floor_val}")
            endif()
            if(floor_val LESS 0)
                math(EXPR final_val "-1 * ${floor_val}")
            else()
                set(final_val "${floor_val}")
            endif()
            set(sanitized_val "${final_val}")
        else()
            # If tick_str is empty or non-numeric (and not "0"), simply use 0 without warning.
            if(NOT tick_str STREQUAL "")
                message(WARNING "[ThreadPool] Request tick '${tick_str}' not numeric; using 0")
            endif()
        endif()
        
        if(_unified_tick STREQUAL "None")
            set(_unified_tick "${sanitized_val}")
        else()
            if(NOT _unified_tick EQUAL 0)
                math(EXPR current_tick "${_unified_tick}")
                if(sanitized_val GREATER current_tick)
                    set(_unified_tick "${sanitized_val}")
                endif()
            endif()
        endif()
    endforeach()
    
    if(_unified_tick STREQUAL "None")
        set(_unified_tick 0)
    endif()
    
    # Save final picks.
    set(THREADPOOL_MODE ${_unified_mode} CACHE INTERNAL "Unified final mode bool" FORCE)
    set(THREADPOOL_ADOPTIVE_TICK ${_unified_tick} CACHE INTERNAL "Unified final tick" FORCE)
    
    if(_unified_mode STREQUAL "1")
        set(mode_str "PRIORITY")
    else()
        set(mode_str "STANDARD")
    endif()
    
    message(STATUS "[${PROJECT_NAME}] Final Unified Configuration: Mode = ${mode_str}, TICK = ${THREADPOOL_ADOPTIVE_TICK}")
    
    set(_THREADPOOL_UNIFIED "ON" CACHE INTERNAL "Unification done" FORCE)
endfunction()
#------------------------------------------------------------------------------------------