/// @file test.cpp
/// @brief Implementation of a basic test harness
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <smallcxx/test.hpp>

using namespace std;

void
test_assert(unsigned int& failures, unsigned int& successes,
            const char *file, int line, const char *function,
            bool cond, const char *format, ...)
{
    if(cond) {
        logMessage(LOG_INFO, file, line, function, "Test passed");
        ++successes;
        return;
    }

    ++failures;

    char cstr[PIPE_BUF + 1];

    va_list args;
    va_start(args, format);
    const auto charsWritten = vsnprintf(cstr, sizeof(cstr), format, args);
    va_end(args);

    if(charsWritten <= 0) {
        cstr[0] = '\0';   // valid string, even if vsnprintf failed. // LCOV_EXCL_LINE
        // coverage excluded because I don't know off-hand how to reliably
        // trigger this case.
    }

    logMessage(LOG_ERROR, file, line, function, "Test failure: %s", cstr);
}

string
testDataFilename(const char *filename)
{
#ifdef SRCDIR
    string retval(SRCDIR);
    retval += '/';
#else
    string retval;
#endif
    return retval + filename;
}
