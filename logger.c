#include <stdio.h>

#include "logger.h"

void sa_log(sa_log_type type, const char msg0[], const char msg1[]) {
    switch(type)
    {
#ifndef SA_NO_ERROR_LOGS
    case ERROR:
        printf("\e[1;31m[  ERROR   ] \e[0m %s %s\n", msg0, msg1);
        break;
#endif
#ifndef SA_NO_WARNING_LOGS
    case WARNING:
        printf("\e[1;33m[  WARNING ] \e[0m %s %s\n", msg0, msg1);
        break;
#endif
#ifndef SA_NO_DEBUG_LOGS
    case DEBUG:
        printf("\e[1;35m[  DEBUG   ] \e[0m %s %s\n", msg0, msg1);
        break;
#endif
    default:
        break;
    }
    fflush(stdout);
}