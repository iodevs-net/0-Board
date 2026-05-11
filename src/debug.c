// SPDX-License-Identifier: MIT — see LICENSE file

#include "debug.h"

#ifdef DEBUG
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>

static FILE *log_stream = NULL;

void debug_init(const char *log_file) {
    if (log_file) {
        log_stream = fopen(log_file, "a");
    }
    if (!log_stream) {
        log_stream = stderr;
    }
    
    LOG_INFO("--- DEBUG SESSION STARTED ---");
}

void debug_log(const char *level, const char *file, int line, const char *fmt, ...) {
    if (!log_stream) return;

    time_t now;
    time(&now);
    struct tm *t = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

    fprintf(log_stream, "[%s] [%s] (%s:%d) ", time_str, level, file, line);
    
    va_list args;
    va_start(args, fmt);
    vfprintf(log_stream, fmt, args);
    va_end(args);
    
    fprintf(log_stream, "\n");
    fflush(log_stream);
}

void debug_cleanup(void) {
    if (log_stream && log_stream != stderr) {
        LOG_INFO("--- DEBUG SESSION ENDED ---");
        fclose(log_stream);
    }
    log_stream = NULL;
}

#endif
