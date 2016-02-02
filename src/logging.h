//
// Created by David Nugent on 2/02/2016.
// Logging functions & interface

#ifndef OPSISD_LOGGING_H
#define OPSISD_LOGGING_H

#include <stddef.h>

enum Verbosity {
    V_NONE =0,
    V_FATAL,
    V_CRITICAL,
    V_ERROR,
    V_DEFAULT = V_ERROR,
    V_WARN,
    V_INFO,
    V_DEBUG,
    V_TRACE
};

enum LogOption {
    LOG_ECHO    =0x01,          // Also log to:
    LOG_STDERR  =0x02,          //  stderr (stdout is default)
    LOG_SYNC    =0x04,          // Force sync after each write
    LOG_UTC     =0x08,          // Log time in UTC
    LOG_FILE    =0x10,          // Log to file
    LOG_NOECHO  =0x20,          // Don't echo log to stdout/stderr
};


// Simplified log4j style interface without the overhead of hierarchical loggers,
// configurable handlers and formatters etc. etc.

extern void log_init(int flags, enum Verbosity verbosity, char const *logpath);
extern void log_rotate();               // close current log (if any), start a new one
extern char const *log_name();          // current log name (NULL if none)

// Logging functions
extern void log_log(enum Verbosity level, char const *fmt, ...) __attribute__((format (printf, 2, 3)));

extern void log_fatal(char const *fmt, ...) __attribute__((format (printf, 1, 2)));
extern void log_critical(char const *fmt, ...) __attribute__((format (printf, 1, 2)));
extern void log_error(char const *fmt, ...) __attribute__((format (printf, 1, 2)));
extern void log_warning(char const *fmt, ...) __attribute__((format (printf, 1, 2)));
extern void log_info(char const *fmt, ...) __attribute__((format (printf, 2, 2)));
extern void log_debug(char const *fmt, ...) __attribute__((format (printf, 1, 2)));
extern void log_trace(char const *fmt, ...) __attribute__((format (printf, 1, 2)));

#endif //OPSISD_LOGGING_H
