/// @file logging.hpp
/// @brief Header for logging library
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021--2022 Christopher White
///
/// Terminology: messages are logged if the "message level" is less than
/// or equal to the "domain level".
///
/// There can be any number of log-message domains, each identified by an
/// arbitrary non-empty string.  (However, strings starting with `" "` (a
/// space) are reserved for use by smallcxx/logging.)  The default domain
/// is called `default`.
///
/// To define your own domain for messages in a source file, `#define` @c
/// SMALLCXX_LOG_DOMAIN to a string constant before you `#include` this file.
/// For example:
///
/// ```
/// #define SMALLCXX_LOG_DOMAIN "myDomain"
/// #include <smallcxx/logging.hpp>
/// ```
///
/// #### Special domains
///
/// - As noted above, domains starting with a space character are reserved.
///   setLogLevel() will refuse to set the level for such a domain.
/// - Domains starting with a plus sign (`+`) are "explicit" domains: they
///   are silent unless you expressly set a value for them.
///
/// #### Thanks
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

/// Default log domain.
#define SMALLCXX_DEFAULT_LOG_DOMAIN "default"

#ifndef SMALLCXX_LOG_DOMAIN
#define SMALLCXX_LOG_DOMAIN SMALLCXX_DEFAULT_LOG_DOMAIN
#endif

/// The string we will use as a hash key for this file's messages
static const std::string SMALLCXX_LOG_DOMAIN_NAME(SMALLCXX_LOG_DOMAIN);

/// @brief Log levels.
/// @warning Keep this in sync with g_levelnames in logging.cpp!
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

    LOG_MIN = LOG_ERROR,    ///< Used for range checks (not an actual level)
    LOG_MAX = LOG_SNOOP,    ///< Used for range checks (not an actual level)

    /// A special level: print, to stdout, only the message.
    /// Use this for messages you want LOG_SILENT to suppress.
    LOG_PRINT,

    /// A special level: as LOG_PRINT, but to stderr.
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

/// Main logging macro.  A newline will be appended to the message.
/// Usage example: `LOG_F(INFO, "foo %s", some_string);`
///
/// When using #LOG_PRINT (print to stdout) and #LOG_PRINTERR (print to stderr),
/// @p format must be a string literal.
///
/// @note `LOG_F(SILENT, ...)` is forbidden.
///
/// @param[in] level - the log level.  A `LOG_FOO` constant minus `LOG_`.
/// @param[in] format - a printf format string
/// @param[in] ... - args to the format string
#define LOG_F(level, format, ...) \
    LOG_F_DOMAIN(SMALLCXX_LOG_DOMAIN_NAME, level, format, ## __VA_ARGS__);

/// Log, with a particular log domain.
#define LOG_F_DOMAIN(domain, level, format, ...) \
    do { \
        /* forbid LOG_SILENT */ \
        static_assert( ( \
                ( (LOG_##level >= LOG_MIN) && (LOG_##level <= LOG_MAX) ) || \
                ( LOG_##level == LOG_PRINT ) || \
                ( LOG_##level == LOG_PRINTERR ) \
                ), "Invalid log level for LOG_F"); \
        /* print it */ \
        const auto domainLevel___ = getLogLevel(domain); \
        if(domainLevel___ != LOG_SILENT) { \
            if(LOG_##level == LOG_PRINT) { \
                printf(format "\n", ## __VA_ARGS__); \
            } else if(LOG_##level == LOG_PRINTERR) { \
                fprintf(stderr, format "\n", ## __VA_ARGS__); \
            } else { \
                logMessage(domain, LOG_##level, __FILE__, __LINE__, __func__, \
                        (format), ## __VA_ARGS__); \
            } \
        } \
    } while(0)

/// Set log level for @p domain to @p newLevel.
/// @param[in]  newLevel - New level.  Must be LOG_SILENT, or between
///     LOG_MIN and LOG_MAX (inclusive).  In particular, you may not set
///     the level to LOG_PRINT or LOG_PRINTERR.
/// @param[in]  domain - Which log domain to apply @p newLevel to.
///     Must be non-empty.
///
/// @warning Some log messages may be dropped when the level changes.
/// @throws std::runtime_error if you try to set LOG_PRINT or LOG_PRINTERR.
void setLogLevel(LogLevel newLevel,
                 const std::string& domain = SMALLCXX_DEFAULT_LOG_DOMAIN);

/// Clip @p level to [LOG_SILENT]+[LOG_MIN, LOG_MAX].  A convenience function.
LogLevel clipLogLevel(LogLevel level);

/// Get the current log level for @p domain.
/// @param[in]  domain - The log domain of interest.  Must be non-empty.
LogLevel getLogLevel(const std::string& domain = SMALLCXX_DEFAULT_LOG_DOMAIN);

/// Set the verbosity for all log domains based on `$V`.  If `$V` exists and
/// is a decimal >=1, the domain level is set to `INFO + $V`
/// (e.g., 1 = LOG_DEBUG, 2 = LOG_LOG).
///
/// @param[in]  detailEnvVarName - if given, non-NULL, and nonempty, the name
///     of a GStreamer-style log-level variable to be used instead of `$V` if
///     the given variable exists.
///     In the log-level variable, the default domain is `default`.
void setVerbosityFromEnvironment(const char *detailEnvVarName = nullptr);

/// Set all domains to silent, **except** for reserved domains (starting with
/// #DOMAIN_SIGIL_RESERVED).  Reserved domains are reset to LOG_INFO.
///
/// Calling this function does not lock the levels.  They can be
/// changed afterwards by calling setLogLevel() or
/// setVerbosityFromEnvironment().
void silenceLog();

#endif // LOGGING_HPP_
