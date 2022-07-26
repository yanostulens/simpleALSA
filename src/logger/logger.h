#ifndef SIMPLEALSALOGGER_H
#define SIMPLEALSALOGGER_H

#include "../config.h"

#define SA_LOG_2_ARGS(type, msg0)        sa_log(type, msg0, "")
#define SA_LOG_3_ARGS(type, msg0, msg1)  sa_log(type, msg0, msg1)

#define GET_4TH_ARG(arg1, arg2, arg3, arg4, ...) arg4
#define SA_LOG_MACRO_CHOOSER(...) \
    GET_4TH_ARG(__VA_ARGS__, SA_LOG_3_ARGS, \
                SA_LOG_2_ARGS, )

#if defined SA_NO_LOGS
    #define SA_LOG(...) ((void) 0)
#else
    #define SA_LOG(...) SA_LOG_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)
#endif

/**
 * @brief enum to identify different types of logs
 *
 */
typedef enum
{
    DEBUG   = 0,
    WARNING = 1,
    ERROR   = 2
} sa_log_type;

void sa_log(sa_log_type type, const char msg0[], const char msg1[]);

#endif  // SIMPLEALSALOGGER_H