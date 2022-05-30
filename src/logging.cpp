/// @file logging.cpp
/// @brief Implementation of logging routines
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <sstream>
#include <stdarg.h>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <unordered_map>

#include "smallcxx/common.hpp"
#include "smallcxx/logging.hpp"

#define SMALLCXX_USE_CHOMP
#include "smallcxx/string.hpp"

using namespace std;

// === Constants and data ================================================

/// Human-readable names of the log levels.
/// Each is <= 5 chars for the `%-5s` in logMessage().
/// @warning Keep this in sync with enum LogLevel!
static const char *g_levelnames[] = {
    "XXXXX",    // LOG_SILENT
    "ERROR",
    "WARN",
    "FIXME",
    "Info",
    "Debug",
    "Log",
    "trace",
    "peek",
    "snoop",
};

/// Log level with a default
struct LogLevelWithDefault {
    static LogLevel defaultLevel;
    LogLevel l; ///< the actual level
    LogLevelWithDefault(): l(defaultLevel) {}
    LogLevelWithDefault(LogLevel newl): l(newl) {}

    /// @name Other
    /// @{
    LogLevelWithDefault(const LogLevelWithDefault&) = default;
    LogLevelWithDefault& operator=(const LogLevelWithDefault&) = default;
    LogLevelWithDefault(LogLevelWithDefault&& other) = default;
    LogLevelWithDefault& operator=(LogLevelWithDefault&& other) = default;
    /// @}
};
LogLevel LogLevelWithDefault::defaultLevel = LOG_INFO;


/// Type to hold current domain levels.
/// Myers singleton since we need it during startup.
struct LogLevelHolder {
    using MapType = unordered_map<string, LogLevelWithDefault>;
    static MapType&
    levels()
    {
        static MapType singleton;
        return singleton;
    }

    /// Accessor so you don't have to say "::levels()" all the time
    MapType::mapped_type&
    operator[](const MapType::key_type& k)
    {
        return levels()[k];
    }
}; // class LogLevelHolder

/// Current log levels
LogLevelHolder g_currSystemLevels;

// Terminal colors
static const char RED[] = "\e[31;1m";       ///< Color for errors
static const char YELLOW[] = "\e[33;1m";    ///< Color for warnings
static const char WHITE[] = "\e[37;1m";     ///< Color for fix-me messages
static const char NORMAL[] = "\e[37;0m";
static const char *PIDCOLORS[] = {  // not any of the above, to avoid confusion
    "\e[30;1m", // bold => gray
    "\e[32m",
    "\e[34;1m", // bold because blue on black is hard to read on my terminal
    "\e[35m",
    "\e[36m",
};

/// Size of the buffers used in vlogMessage()
static const size_t LOGBUF_NBYTES = 256;
static_assert(LOGBUF_NBYTES <= PIPE_BUF, "Log messages are not atomic");

/// Where we write log messages
static const int LOG_FD = STDERR_FILENO;

// === Routines ==========================================================

/// @name Writing messages
/// @{

/// @note Assumes write(2) calls of <= PIPE_BUF bytes are atomic.
void
logMessage(const std::string& domain,
           LogLevel msgLevel, const char *file, const int line,
           const char *function,
           const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vlogMessage(domain, msgLevel, file, line, function, format, args);
    va_end(args);
}

void
vlogMessage(const std::string& domain,
            LogLevel msgLevel, const char *file, const int line,
            const char *function,
            const char *format, va_list args)
{
    static bool tty = isatty(LOG_FD) && !getenv("NO_COLOR");
    static intmax_t pid = getpid();
    static const char *pidcolor = tty ? PIDCOLORS[pid % ARRAY_SIZE(PIDCOLORS)] : "";
    static const char *endcolor = tty ? NORMAL : "";

    int __attribute__((unused)) ignore_return_value;
    char preamble[LOGBUF_NBYTES];
    char msg[LOGBUF_NBYTES];
    char whole[LOGBUF_NBYTES];
    int charsWritten;

    // Accept the possibility of missing a log message around the time
    // the level changes.
    if(msgLevel > g_currSystemLevels[domain].l) {
        return;
    }

    struct timespec ts;
    clock_gettime(CLOCK_BOOTTIME, &ts);
    const double boottime = ts.tv_sec + (double)ts.tv_nsec / 1e9;

    charsWritten = snprintf(preamble, sizeof(preamble), "%.9f", boottime);

    if(charsWritten <= 0 || (size_t)charsWritten >= sizeof(preamble)) {
        // Uncovered --- I don't know any way to cause this to happen so I can test it
        preamble[0] = '\0';    //keep going without a timestamp    // LCOV_EXCL_LINE
    }

    string stimestamp(preamble);    // max 16 chars
    if(stimestamp.length() > 16) {
        stimestamp = stimestamp.substr(stimestamp.length() - 16);
    }

    // Level
    const char *levelname =
        ((msgLevel < LOG_MIN) || (msgLevel > LOG_MAX)) ? "" : g_levelnames[msgLevel];
    const char *bodycolor = !tty ? "" : (
                                (msgLevel == LOG_ERROR) ? RED :
                                (msgLevel == LOG_WARNING) ? YELLOW :
                                (msgLevel == LOG_FIXME) ? WHITE :
                                NORMAL
                            );

    // Assemble preamble
    charsWritten = snprintf(preamble, sizeof(preamble),
                            "[%16.16s] %s%-8" PRIdMAX "%s %-5.5s %20.20s:%-4d %-20.20s ",
                            stimestamp.c_str(),
                            pidcolor, pid, bodycolor, levelname, file, line, function);

    if(charsWritten <= 0 || (size_t)charsWritten >= sizeof(preamble)) {
        // LCOV_EXCL_START
        // Uncovered --- I don't know any way to cause this to happen so I can test it
        const char msg[] = "Dropped log message (preamble error)\n";
        ignore_return_value = write(LOG_FD, msg, sizeof(msg));
        return;
    } // LCOV_EXCL_STOP

    // The user's message
    charsWritten = vsnprintf(msg, sizeof(msg), format, args);

    if(charsWritten <= 0) { // accept message truncation
        // LCOV_EXCL_START
        // Uncovered; same reason as above
        const char msg[] = "Dropped log message (message error)\n";
        ignore_return_value = write(LOG_FD, msg, sizeof(msg));
        return;
    } // LCOV_EXCL_STOP

    chomp(msg);

    // Put it together
    int roomForMessage = sizeof(whole) - strlen(preamble)
                         - 2 - strlen(endcolor);   // -2 = room for "\n\0"
    charsWritten = snprintf(whole, sizeof(whole), "%s%-.*s%s\n", preamble,
                            roomForMessage, msg, endcolor);

    if(charsWritten <= 0) {
        // LCOV_EXCL_START
        // Uncovered; same reason as above
        const char msg[] = "Dropped log message (assembly error)\n";
        ignore_return_value = write(LOG_FD, msg, sizeof(msg));
        return;
    } // LCOV_EXCL_STOP

    ignore_return_value = write(LOG_FD, whole, charsWritten);
} //log_message()

/// @}

/// @name Getting and setting the log level from code
/// @{

LogLevel
clipLogLevel(LogLevel level)
{
    if(level == LOG_SILENT) {
        return LOG_SILENT;
    }

    if(level < LOG_MIN) {
        return LOG_MIN;
    }

    if(level > LOG_MAX) {
        return LOG_MAX;
    }

    return level;
}

void
setLogLevel(LogLevel newLevel, const std::string& domain)
{
    throw_assert(!domain.empty());

    if(domain[0] == ' ') {
        throw domain_error("Logging domains starting with a space are reserved");
    }

    if(newLevel == LOG_SILENT) {
        g_currSystemLevels[domain].l = newLevel;
        return; // *** EXIT POINT ***
    }

    if( (newLevel == LOG_PRINT) || (newLevel == LOG_PRINTERR) ) {
        throw domain_error(STR_OF
                           << "Ignoring attempt to set invalid log level for "
                           << domain);
    }

    newLevel = clipLogLevel(newLevel);

    g_currSystemLevels[domain].l = newLevel;
}

LogLevel
getLogLevel(const std::string& domain)
{
    throw_assert(!domain.empty());
    return g_currSystemLevels[domain].l;
}

/// @}

/// @name Setting the log level from the environment
/// @{

static int
parseNonNegInt(const char *c_str)
{
    long delta = strtol(c_str, NULL, 10);
    if( (delta < 0) || (delta == LONG_MAX) || (delta > INT_MAX) ) {
        throw domain_error("Invalid level value (must be non-negative integer)");
    }
    return delta;
}

static int
parsePosInt(const char *c_str)
{
    int retval = parseNonNegInt(c_str);
    if(retval == 0) {
        throw domain_error("Invalid level value (must be positive integer)");
    }
    return retval;
}

static LogLevel
parseLevel(const std::string& str)
{
    int posint = parseNonNegInt(str.c_str());

    // setLogLevel will handle clipping to the range.
    return (LogLevel)posint;
}

/// Try to parse a "<domain>:<value>" pair extracted from an env var
static void
setLevelFromStrings(const std::string& domain, const std::string& value)
{
    LogLevel level = clipLogLevel(parseLevel(value));

    if(domain == "*") {
        LogLevelWithDefault::defaultLevel = level;
    } else if(!domain.empty()) {
        setLogLevel(level, domain);
    } else {
        throw domain_error("Invalid logging domain");
    }

}

/// @return True if @p detailEnvVarName was parsed successfully
static bool
parseDetail(const char *detailEnvVarName)
{
    char *detail;
    if(!detailEnvVarName || (detailEnvVarName[0] == '\0') ||
            ((detail = getenv(detailEnvVarName)) == nullptr)) {
        return false;
    }

    enum { get_domain, get_value } state;
    state = get_domain;
    string domain, value;
    for(const auto c : string(detail)) {
        if(isspace(c)) {
            continue;
        }
        switch(state) {
        case get_domain:
            if(c == ':') {
                state = get_value;
                continue;
            }
            domain += c;
            break;

        case get_value:
            if(c == ',') {
                // Finish
                setLevelFromStrings(domain, value);

                // Set up for the next one, if any
                state = get_domain;
                domain = "";
                value = "";
                break;
            }
            value += c;
            break;

        default:
            throw logic_error("Invalid state while trying to parse logging command!");
            break;
        }
    } //foreach char

    // Reject domain without value
    if( ( (state == get_domain) && !domain.empty() ) ||
            ( (state == get_value) && value.empty() )
      ) {
        throw domain_error(STR_OF << "No value given for domain " << domain);
    }

    // Last one, if any
    if(!domain.empty() && !value.empty()) {
        setLevelFromStrings(domain, value);
    }

    return true;
}

static void
parseV()
{
    const char *c_str = getenv("V");
    if(!c_str) {
        return;
    }

    int delta = parsePosInt(c_str);

    LogLevelWithDefault::defaultLevel = clipLogLevel((LogLevel)(LOG_INFO + delta));
    setLogLevel(LogLevelWithDefault::defaultLevel);
}

void
setVerbosityFromEnvironment(const char *detailEnvVarName)
{
    try {
        if(parseDetail(detailEnvVarName)) {
            return;
        }

        parseV();
    } catch(exception& e) {
        fprintf(stderr, "Could not parse verbosity: %s\n", e.what());
    } catch(...) {
        fprintf(stderr, "Unexpected error while trying to parse verbosity\n");
    }
}

void
silenceLog()
{
    LogLevelWithDefault::defaultLevel = LOG_SILENT;
    g_currSystemLevels.levels().clear();
    setLogLevel(LOG_SILENT);
}

/// @}
