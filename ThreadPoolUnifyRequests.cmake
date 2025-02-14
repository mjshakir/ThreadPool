###
# ThreadPoolUnifyRequests.cmake
#
# This file provides:
#   (1) threadpool_request_config(MODE_BOOL TICK)
#       - Submodules or top-level code call this to request a certain mode (0 or 1) + adoptive tick
#   (2) _threadpool_unify_requests()
#       - Internal function that unifies all requests:
#           * If any submodule has MODE=1 => final=1 (PRIORITY), else 0 (STANDARD).
#           * TICK => the max among all requests after floor(abs(...)).
#       - If no requests, we fallback to:
#           THREADPOOL_DEFAULT_MODE (a string "0" or "1")
#           BUILD_THREADPOOL_DEFAULT_ADOPTIVE_TICK (any numeric string).
#
###
#------------------------------------------------------------------------------------------
# Internal cache variable to collect requests:
#   e.g. "0;1000000;1;2000000;1;500000"
if(NOT DEFINED _THREADPOOL_REQUEST_LIST)
    set(_THREADPOOL_REQUEST_LIST "" CACHE INTERNAL "Accumulated requests for ThreadPool" FORCE)
endif()

#------------------------------------------------------------------------------------------
# 1) threadpool_request_config(MODE_BOOL, TICK)
#
#    Example usage by submodules:
#       threadpool_request_config("0" "1000000")  # => STANDARD, tick=1e6
#       threadpool_request_config("1" "500000")   # => PRIORITY, tick=5e5
#
function(threadpool_request_config mode tick)
    set(_prev "${_THREADPOOL_REQUEST_LIST}")
    set(_new  "${_prev};${mode};${tick}")
    set(_THREADPOOL_REQUEST_LIST "${_new}" CACHE INTERNAL "Accumulated requests for ThreadPool" FORCE)

    message(STATUS "[ThreadPool] Submodule requests => Mode=${mode}, Tick=${tick}")
endfunction()

#------------------------------------------------------------------------------------------
# 2) _threadpool_unify_requests()
#
#    Called once in the main CMakeLists to parse all requests and unify:
#        - If ANY request is "1" => final=1, else 0
#        - TICK => pick the max after floor(abs(...))
#    If no requests come in, fallback to:
#        THREADPOOL_DEFAULT_MODE (e.g. "0" or "1")
#        BUILD_THREADPOOL_DEFAULT_ADOPTIVE_TICK (e.g. "1000000")
#
function(_threadpool_unify_requests)
    # If no requests => fallback to defaults
    if(_THREADPOOL_REQUEST_LIST STREQUAL "")
        set(_REQUESTS "${THREADPOOL_DEFAULT_MODE};${BUILD_THREADPOOL_DEFAULT_ADOPTIVE_TICK}")
    else()
        set(_REQUESTS "${_THREADPOOL_REQUEST_LIST}")
    endif()

    # Convert from semicolon-based string => real list
    separate_arguments(_REQUESTS)

    # Start with standard=0, tick=0
    set(_unified_mode "0")
    set(_unified_tick "0")

    list(LENGTH _REQUESTS _len)
    math(EXPR _pairs "${_len} / 2")

    set(_i 0)
    while(_i LESS _pairs)
        math(EXPR _mode_idx "${_i} * 2")
        math(EXPR _tick_idx "${_i} * 2 + 1")
        list(GET _REQUESTS ${_mode_idx} this_mode_str)
        list(GET _REQUESTS ${_tick_idx} this_tick_str)

        # A) If ANY mode=1 => unify=1
        if(this_mode_str STREQUAL "1")
            set(_unified_mode "1")
        endif()

        # B) TICK => parse float => floor => abs => unify by max
        # Step 1: Clean out weird characters except digits, sign, decimal
        string(REGEX REPLACE "[^0-9\\-\\+\\.]" "" this_tick_clean "${this_tick_str}")

        # We'll parse with math(EXPR), then do floor(), then abs()
        # If it fails to parse => fallback=0
        set(sanitized_val "0")  # default if error

        # Check if it at least looks numeric: e.g. +12, -3.5, 99, 0.0
        if(this_tick_clean MATCHES "^[+-]?[0-9]+(\\.[0-9]+)?$")
            # parse as a float
            math(EXPR parsed_float "0 + ${this_tick_clean}")

            # floor => integer
            set(floor_val "${parsed_float}")
            if(floor_val MATCHES "^-?[0-9]+\\.[0-9]+$")
                string(REGEX REPLACE "\\.[0-9]+$" "" floor_val "${floor_val}")
            endif()

            # abs => non-negative
            if(floor_val LESS 0)
                math(EXPR final_val "-1 * ${floor_val}")
            else()
                set(final_val "${floor_val}")
            endif()

            set(sanitized_val "${final_val}")
        else()
            message(WARNING "[ThreadPool] Tick '${this_tick_str}' not numeric => using 0")
        endif()

        # Compare with current
        math(EXPR current_tick_val "${_unified_tick}")
        if(sanitized_val GREATER current_tick_val)
            set(_unified_tick "${sanitized_val}")
        endif()

        math(EXPR _i "${_i} + 1")
    endwhile()

    # Store final picks
    set(THREADPOOL_MODE      ${_unified_mode} CACHE INTERNAL "Unified final mode bool" FORCE)
    set(THREADPOOL_ADOPTIVE_TICK  ${_unified_tick} CACHE INTERNAL "Unified final tick"       FORCE)

    if(_unified_mode STREQUAL "1")
        set(mode_str "PRIORITY")
    else()
        set(mode_str "STANDARD")
    endif()

    message(STATUS "[${PROJECT_NAME}] ThreadPool Unified Configuration: Mode = ${mode_str}, TICK = ${THREADPOOL_ADOPTIVE_TICK}")
endfunction()
#-----------------------------------------------------------------------------------------