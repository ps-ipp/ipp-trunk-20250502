#ifndef PS_EXIT_H
#define PS_EXIT_H

/// @addtogroup SysUtils System Utilities
/// @{

/// Exit status codes
///
/// These provide a bit finer granularity compared to success/fail
typedef enum {
    PS_EXIT_SUCCESS = 0,                ///< Successful termination; matches EXIT_SUCCESS
    PS_EXIT_UNKNOWN_ERROR = 1,          ///< Error of unknown nature; matches EXIT_FAILURE
    PS_EXIT_SYS_ERROR,                  ///< Error with a system call
    PS_EXIT_CONFIG_ERROR,               ///< Error with configuration
    PS_EXIT_PROG_ERROR,                 ///< Error in programming (look also for aborts)
    PS_EXIT_DATA_ERROR,                 ///< Error with data
    PS_EXIT_TIMEOUT_ERROR,              ///< Error due to timeout
} psExit;

/// @}
#endif
