/// @file logging.hpp
/// @brief Header for logging library
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White
///
/// Terminology: messages are logged if the "message level" is less than
/// or equal to the "system level".
///
/// There can be any number of log-message domains, each identified by an
/// arbitrary string.  Strings starting with `" "` (a space) are reserved for
/// use by smallcxx/logging.  To define your own domain for messages in
/// a source file:
///
/// ```
/// #define SMALLCXX_LOG_DOMAIN "myDomain"
/// #include <smallcxx/logging.hpp>
/// ```
///
/// This logging library is inspired by (but not copied from):
/// - [Loguru](https://github.com/emilk/loguru) by
///   Emil Ernerfeldt, licensed public domain.
/// - [GStreamer logging](https://gstreamer.freedesktop.org/documentation/tutorials/basic/debugging-tools.html)

#ifndef LOGGING_HPP_
#define LOGGING_HPP_

#include <stdarg.h>
#include <stdio.h>
#include <string>

/// Default log domain.  To use your own domain, `#define`
/// @c SMALLCXX_LOG_DOMAIN to a string constant before `#includ`ing this file
#define SMALLCXX_DEFAULT_LOG_DOMAIN ""

#ifndef SMALLCXX_LOG_DOMAIN
#define SMALLCXX_LOG_DOMAIN SMALLCXX_DEFAULT_LOG_DOMAIN
#endif

/// The string we will use as a hash key for this file's messages
static const std::string SMALLCXX_LOG_DOMAIN_NAME(SMALLCXX_LOG_DOMAIN);

// Log levels.  Errors are always visible; other messages can be suppressed
// by calling setLogLevel().
// @warning Keep this in sync with g_levelnames in logging.cpp!
enum LogLevel {
    LOG_SILENT, ///< Nothing prints
    LOG_ERROR,
    LOG_WARNING,
    LOG_FIXME,
    LOG_INFO,   ///< default log level
    LOG_DEBUG,
    LOG_LOG,    ///< GStreamer-inspired naming
    LOG_TRACE,
    LOG_PEEK,
    LOG_SNOOP,

    LOG_MIN = LOG_ERROR,    ///< Used for range checks
    LOG_MAX = LOG_SNOOP,    ///< Used for range checks

    /// A special level: print, to stdout, only the message.
    /// Use this for messages you want LOG_SILENT to suppress.
    LOG_PRINT,

    // A special level: as LOG_PRINT, but to stderr.
    /// Use this for messages you want LOG_SILENT to suppress.
    LOG_PRINTERR,
};

/// Log a message to stderr.
/// Messages are arbitrarily limited to 256 bytes.
/// @param[in]  domain - A string describing this log domain
/// @param[in]  msgLevel - The log level of this message
/// @param[in]  file - file from which this message was printed
/// @param[in]  line - line number in @p file
/// @param[in]  function - name of function that called logMessage()
/// @param[in]  format - printf format
/// @param[in]  ... - args to @p format
void logMessage(const std::string& domain,
                LogLevel msgLevel, const char *file, const int line,
                const char *function,
                const char *format, ...) __attribute__(( format(printf, 6, 7) ));

/// As logMessage(), but accepts a va_list.
/// @pre @p line <= 9999
/// @note   Assumes the PID never changes --- don't put this in a shared
///         library unless you change that assumption!
void vlogMessage(const std::string& domain,
                 LogLevel msgLevel, const char *file, const int line,
                 const char *function,
                 const char *format, va_list args);

/// Main logging macro.
/// Usage example: `LOG_F(INFO, "foo %s", some_string);`
/// @note `LOG_F(SILENT, ...)` is forbidden.
#define LOG_F(level, format, ...) \
    LOG_F_DOMAIN(SMALLCXX_LOG_DOMAIN_NAME, level, (format), ## __VA_ARGS__);

/// Log, with a particular log domain.
#define LOG_F_DOMAIN(domain, level, format, ...) \
    do { \
        /* forbid LOG_SILENT */ \
        static_assert( \
                ( (LOG_##level >= LOG_MIN) && (LOG_##level <= LOG_MAX) ) || \
                ( LOG_##level == LOG_PRINT ) || \
                ( LOG_##level == LOG_PRINTERR ) \
        ); \
        /* print it */ \
        if(LOG_##level == LOG_PRINT) { \
            if(getLogLevel(domain) != LOG_SILENT) { \
                printf((format), ## __VA_ARGS__); \
            } \
        } else if(LOG_##level == LOG_PRINTERR) { \
            if(getLogLevel(domain) != LOG_SILENT) { \
                fprintf(stderr, (format), ## __VA_ARGS__); \
            } \
        } else { \
            logMessage(domain, LOG_##level, __FILE__, __LINE__, __func__, \
                    (format), ## __VA_ARGS__); \
        } \
    } while(0)

/// Set log level for @p domain to @p newLevel.
/// @param[in]  newLevel - New level.  Must be LOG_SILENT, or between
///     LOG_MIN and LOG_MAX (inclusive).  In particular, you may not set
///     the level to LOG_PRINT or LOG_PRINTERR.
/// @param[in]  domain - Which log domain to apply @p newLevel to.
///
/// @warning Some log messages may be dropped when the level changes.
/// @throws std::runtime_error if you try to set LOG_PRINT or LOG_PRINTERR.
void setLogLevel(LogLevel newLevel,
                 const std::string& domain = SMALLCXX_DEFAULT_LOG_DOMAIN);

/// Get the current log level
LogLevel getLogLevel(const std::string& domain = SMALLCXX_DEFAULT_LOG_DOMAIN);

/// Set the verbosity for all log domains based on $V.  If $V exists and
/// is a decimal >=1, the system level is set to INFO + $V (e.g., 1 = LOG_DEBUG,
/// 2 = LOG_LOG).
/// @note Does not set verbosities for all log domains.
/// @todo implement a `GST_DEBUG`-style log-level selector.
/// @warning Must be called before any messages are logged if it is to affect
///     all log domains.
///
/// @param[in]  detailEnvVarName - if given, non-NULL, and nonempty, the name
///     of a GStreamer-style log-level variable to be used instead of `$V` if
///     the given variable exists.
void setVerbosityFromEnvironment(const char *detailEnvVarName = nullptr);

/// Set all levels to silent.  Does not lock them there; they can be changed
/// afterwards by calling setLogLevel() or setVerbosityFromEnvironment().
void silenceLog();

#endif // LOGGING_HPP_
