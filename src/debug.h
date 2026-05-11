// SPDX-License-Identifier: MIT — see LICENSE file

#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#ifdef DEBUG
    // Si estamos en modo DEBUG, definimos las funciones y macros de log
    void debug_init(const char *log_file);
    void debug_log(const char *level, const char *file, int line, const char *fmt, ...);
    void debug_cleanup(void);

    #define LOG_INFO(fmt, ...)  debug_log("INFO",  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_DEBUG(fmt, ...) debug_log("DEBUG", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...) debug_log("ERROR", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define DEBUG_INIT(file)    debug_init(file)
    #define DEBUG_CLEANUP()     debug_cleanup()
#else
    // Si NO estamos en modo DEBUG, las macros se expanden a NADA.
    // El compilador ni siquiera verá estas líneas en el binario final.
    #define LOG_INFO(fmt, ...)  ((void)0)
    #define LOG_DEBUG(fmt, ...) ((void)0)
    #define LOG_ERROR(fmt, ...) ((void)0)
    #define DEBUG_INIT(file)    ((void)0)
    #define DEBUG_CLEANUP()     ((void)0)
#endif

#endif // DEBUG_H
