/// @file logging.hpp
/// @brief Header for logging library
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White
///
/// This file is inspired by (but not copied from):
/// - [Loguru](https://github.com/emilk/loguru) by
///   Emil Ernerfeldt, licensed public domain.
/// - [GStreamer logging](https://gstreamer.freedesktop.org/documentation/tutorials/basic/debugging-tools.html)

#ifndef LOGGING_HPP_
#define LOGGING_HPP_

#include <smallcxx/common.hpp>
#include <stdarg.h>

// Log levels.  Errors are always visible; other messages can be suppressed
// by calling setLogLevel().
enum LogLevel {
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,   ///< default log level
    LOG_DEBUG,
    LOG_LOG,    ///< GStreamer-esque naming because that's what I use at $work ;)
    LOG_TRACE,
    LOG_PEEK,
    LOG_SNOOP,
    LOG_NLEVELS
};

/// Log a message to stderr.
/// Messages are arbitrarily limited to 256 bytes.
void logMessage(LogLevel level, const char *file, const int line,
                const char *function,
                const char *format, ...) __attribute__(( format(printf, 5, 6) ));

/// As logMessage(), but accepts a va_list.
/// @pre \p line <= 9999
/// @note   Assumes the PID never changes --- don't put this in a shared
///         library unless you change that assumption!
void vlogMessage(LogLevel level, const char *file, const int line,
                 const char *function,
                 const char *format, va_list args);

/// Main logging macro.
#define LOG_F(level, format, ...) \
    logMessage(LOG_##level, __FILE__, __LINE__, __func__, (format), ## __VA_ARGS__)

/// Set log level.
/// @warning Some log messages may be dropped when the level changes.
void setLogLevel(LogLevel level);

/// Get the current log level
LogLevel getLogLevel();

/// Set the verbosity based on $V.  If $V exists and is a decimal >=1,
/// the level is set to INFO + $V (e.g., 1 = LOG_DEBUG, 2 = LOG_LOG).
void setVerbosityFromEnvironment();

void chomp(char *str);

#endif // LOGGING_HPP_
