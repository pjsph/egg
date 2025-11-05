#include "logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void elog(log_level level, const char *file, i32 line, const char *msg, ...) {
    const char *levels[5] = { "FATAL", "ERROR", "WARN", "INFO", "DEBUG" };

    char out_msg[32000];
    memset(out_msg, 0, sizeof(out_msg));

    u8 out_msg_idx = 0;
    strcpy(out_msg, levels[level]);
    out_msg_idx += strlen(levels[level]);

    if(file && line >= 0) {
        out_msg_idx += sprintf(out_msg + out_msg_idx, "(%s:%u)", file, line);
    }

    strcpy(out_msg + out_msg_idx, ": ");
    out_msg_idx += 2;

    va_list arg_ptr;
    va_start(arg_ptr, msg);
    out_msg_idx += vsnprintf(out_msg + out_msg_idx, 32000 - out_msg_idx, msg, arg_ptr);
    va_end(arg_ptr);

    const u8 is_error = level <= LOG_LEVEL_WARN;
    if(is_error) {
        fprintf(stderr, "%s\n", out_msg); 
    } else {
        printf("%s\n", out_msg);
    }
}
