/// @file bin/smallcxxlog.cpp
/// @brief Print a log message from the command line
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021--2022 Christopher White

#include <cinttypes>
#include <cstdlib>
#include <stdio.h>

#include <smallcxx/logging.hpp>

using std::atoi;

namespace smallcxx
{
extern intmax_t PidOverride;
}

/// Set smallcxx::PidOverride from a string
/// @note Any failures are silent so as not to interfere with the message
///     we are trying to log.
void
OverridePidTo(const char *str)
{
    char *endptr;
    intmax_t value = std::strtoimax(str, &endptr, 10);
    if(errno != 0) {    // bad value
        return;
    }
    if(endptr && *endptr) { // leftover garbage in the pid
        return;
    }

    smallcxx::PidOverride = value;
}

int
main(int argc, char **argv)
{
    if(argc < 6) {
        fprintf(stderr, "Usage: %s LEV FILE LINE FUNCTION 'MESSAGE' [PID]\n", argv[0]);
        return 2;
    }

    // Set the level so the message will always print (TODO make this an option?)
    setLogLevel(LOG_MAX);

    // Parse cmdline
    LogLevel level = (LogLevel)atoi(argv[1]);     // TODO parse level names
    if(level == LOG_SILENT) {   // if atoi() couldn't parse ...
        level = LOG_FIXME;      // ... print it as fixme.
    }

    const char *file = argv[2];
    const int line = atoi(argv[3]);
    const char *function = argv[4];
    const char *msg = argv[5];

    if(argc >= 7) {
        OverridePidTo(argv[6]);
    }

    logMessage(SMALLCXX_DEFAULT_LOG_DOMAIN, level,
               file, line, function, "%s", msg);
    return 0;
}
