#ifndef LOGGER_H
#define LOGGER_H

#include "defines.h"

#ifdef EDEBUG_MODE
    #define LOG_LEVEL 4
#else
    #define LOG_LEVEL 3
#endif

typedef enum log_level {
    LOG_LEVEL_FATAL = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN  = 2,
    LOG_LEVEL_INFO  = 3,
    LOG_LEVEL_DEBUG = 4,
} log_level;

EAPI void elog(log_level level, const char *file, i32 line, const char *msg, ...);

#define EFATAL(msg, ...) elog(LOG_LEVEL_FATAL, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define EERROR(msg, ...) elog(LOG_LEVEL_ERROR, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define EWARN(msg, ...) elog(LOG_LEVEL_WARN, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define EINFO(msg, ...) elog(LOG_LEVEL_INFO, 0, -1, msg, ##__VA_ARGS__)

#if LOG_LEVEL >= 4
    #define EDEBUG(msg, ...) elog(LOG_LEVEL_DEBUG, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#else
    #define EDEBUG(msg, ...)
#endif

#endif // LOGGER_H
