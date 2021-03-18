/// @file logging.cpp
/// @brief Implementation of logging routines
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <smallcxx/logging.hpp>

using namespace std;

// === Constants and data ================================================

/// Human-readable names of the log levels.
/// Each is <= 5 chars for the `%-5s` in logMessage().
static const char *g_levelnames[] = {
    "ERROR",
    "WARN",
    "Info",
    "Debug",
    "Log",
    "trace",
    "peek",
    "snoop",
};

/// Current log level.
static LogLevel g_currLevel = LOG_INFO;

// Terminal colors
static const char RED[] = "\e[31;1m";
static const char YELLOW[] = "\e[33;1m";
static const char NORMAL[] = "\e[0m";
static const char *PIDCOLORS[] = {  // not red or yellow, to avoid confusion
    "\e[30;1m",
    "\e[32;1m",
    "\e[34;1m",
    "\e[35;1m",
    "\e[36;1m",
    "\e[37;1m",
};

/// Size of the buffers used in vlogMessage()
static const size_t LOGBUF_NBYTES = 256;
static_assert(LOGBUF_NBYTES <= PIPE_BUF, "Log messages are not atomic");

/// Where we write log messages
static const int LOG_FD = STDERR_FILENO;

// === Routines ==========================================================

/// @note Assumes write(2) calls of <= PIPE_BUF bytes are atomic.
void
logMessage(LogLevel level, const char *file, const int line,
           const char *function,
           const char *format, ...)
{
    // Accept the possibility of missing a log message around the time
    // the level changes.
    if(level > g_currLevel) {
        return;
    }

    va_list args;
    va_start(args, format);
    vlogMessage(level, file, line, function, format, args);
    va_end(args);
}

void
vlogMessage(LogLevel level, const char *file, const int line,
            const char *function,
            const char *format, va_list args)
{
    static bool tty = isatty(LOG_FD) && !getenv("NO_COLOR");
    static intmax_t pid = getpid();
    static const char *pidcolor = tty ? PIDCOLORS[pid % ARRAY_SIZE(PIDCOLORS)] : "";

    int __attribute__((unused)) ignore_return_value;
    char preamble[LOGBUF_NBYTES];
    char msg[LOGBUF_NBYTES];
    char whole[LOGBUF_NBYTES];
    int charsWritten;

    // Accept the possibility of missing a log message around the time
    // the level changes.
    if(level > g_currLevel) {
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
        ((level < 0) || (level >= LOG_NLEVELS)) ? "" : g_levelnames[level];
    const char *bodycolor = !tty ? "" : (
                                (level == LOG_ERROR) ? RED :
                                (level == LOG_WARNING) ? YELLOW :
                                NORMAL
                            );

    // Assemble preamble
    charsWritten = snprintf(preamble, sizeof(preamble),
                            "[%16.16s] %s%-8" PRIdMAX "%s %-5.5s %20.20s:%-4d %-20.20s ",
                            stimestamp.c_str(),
                            pidcolor, pid, bodycolor, levelname, file, line, function);

    if(charsWritten <= 0 || (size_t)charsWritten >= sizeof(preamble)) {
        // *INDENT-OFF*
        // Uncovered --- I don't know any way to cause this to happen so I can test it
        const char msg[] = "Dropped log message (preamble error)\n";    // LCOV_EXCL_START
        ignore_return_value = write(LOG_FD, msg, sizeof(msg));
        return;
        // *INDENT-ON*
    } // LCOV_EXCL_STOP

    // The user's message
    charsWritten = vsnprintf(msg, sizeof(msg), format, args);

    if(charsWritten <= 0) { // accept message truncation
        // *INDENT-OFF*
        // Uncovered; same reason as above
        const char msg[] = "Dropped log message (message error)\n"; // LCOV_EXCL_START
        ignore_return_value = write(LOG_FD, msg, sizeof(msg));
        return;
        // *INDENT-ON*
    } // LCOV_EXCL_STOP

    chomp(msg);

    // Put it together
    const char *endcolor = tty ? NORMAL : "";
    int roomForMessage = sizeof(whole) - strlen(preamble)
                         - 2 - strlen(endcolor);   // -2 = room for "\n\0"
    charsWritten = snprintf(whole, sizeof(whole), "%s%-.*s%s\n", preamble,
                            roomForMessage, msg, endcolor);

    if(charsWritten <= 0) {
        // *INDENT-OFF*
        // Uncovered; same reason as above
        const char msg[] = "Dropped log message (assembly error)\n";    // LCOV_EXCL_START
        ignore_return_value = write(LOG_FD, msg, sizeof(msg));
        return;
        // *INDENT-ON*
    } // LCOV_EXCL_STOP

    ignore_return_value = write(LOG_FD, whole, charsWritten);
} //log_message()

void
setLogLevel(LogLevel level)
{
    if(level < 0) {
        level = (LogLevel)0;
    }
    if(level >= LOG_NLEVELS) {
        level = (LogLevel)(LOG_NLEVELS - 1);
    }
    g_currLevel = level;
}

LogLevel
getLogLevel()
{
    return g_currLevel;
}

void
setVerbosityFromEnvironment()
{
    const char *verbosity = getenv("V");
    if(!verbosity) {
        return;
    }

    long delta = strtol(verbosity, NULL, 10);
    if(delta <= 0 || delta == LONG_MAX) {
        return;
    }

    setLogLevel((LogLevel)(LOG_INFO + delta));
}

void
chomp(char *str)
{
    if(str[0] == '\0') {
        return;
    }
    if(str[strlen(str) - 1] == '\n') {
        str[strlen(str) - 1] = '\0';
    }
}

