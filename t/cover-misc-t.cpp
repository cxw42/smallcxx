/// @file cover-misc-t.cpp
/// @brief Coverage for misc. items
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#define SMALLCXX_USE_CHOMP
#include "smallcxx/string.hpp"
#include "smallcxx/test.hpp"

TEST_FILE

using namespace std;

void
test_chomp()
{
    char cbuf[16] = {0};
    chomp(cbuf);
    isstr(cbuf, (""));

    cbuf[0] = '\n';
    chomp(cbuf);
    isstr(cbuf, (""));
}

/// Helper for test_setloglevel
void
invoke_vlogMessage(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vlogMessage(SMALLCXX_DEFAULT_LOG_DOMAIN,
                LOG_LOG, __FILE__, __LINE__, __func__, format, args);
    va_end(args);
}

void
test_setloglevel()
{
    // Valid cases
    setLogLevel((LogLevel) - 1);
    setLogLevel(LOG_MIN);
    cmp_ok(getLogLevel(), ==, LOG_ERROR);
    setLogLevel(LOG_MAX);
    cmp_ok(getLogLevel(), ==, LOG_SNOOP);
    setLogLevel(LOG_MIN);
    cmp_ok(getLogLevel(), ==, LOG_ERROR);
    setLogLevel(LOG_SILENT);
    cmp_ok(getLogLevel(), ==, LOG_SILENT);

    // Invalid cases
    throws_with_msg(
        setLogLevel((LogLevel)(LOG_MAX + 1)),   // == LOG_PRINT
        "Ignoring attempt"
    );
    throws_with_msg(
        setLogLevel(LOG_PRINT),
        "Ignoring attempt"
    );
    throws_with_msg(
        setLogLevel(LOG_PRINTERR),
        "Ignoring attempt"
    );
    cmp_ok(getLogLevel(), ==, LOG_SILENT);
    setLogLevel(LOG_DEBUG);
    cmp_ok(getLogLevel(), ==, LOG_DEBUG);
    LOG_F(LOG, "** If you see this message, there's a bug in logging.cpp! **");
    invoke_vlogMessage("** If you see this message, there's a bug in logging.cpp! **");
}

void
test_env_loglevel()
{
    // V=0 doesn't change log level
    const auto old = getLogLevel();
    cmp_ok(setenv("V", "0", true), ==, 0);
    setVerbosityFromEnvironment();
    cmp_ok(getLogLevel(), ==, old);

    // V not a number doesn't change log level
    cmp_ok(setenv("V", "quux", true), ==, 0);
    setVerbosityFromEnvironment();
    cmp_ok(getLogLevel(), ==, old);

    // Add 1
    cmp_ok(setenv("V", "1", true), ==, 0);
    setVerbosityFromEnvironment();
    cmp_ok(getLogLevel(), ==, LOG_INFO + 1);
}

/// Log an error and a warning, which will be in color if output is going
/// to the tty.  This is for coverage of `bodycolor` in vlogMessage().
void
emit_possibly_colorful_messages()
{
    setLogLevel(LOG_INFO);
    cmp_ok(getLogLevel(), ==, LOG_INFO);
    LOG_F(ERROR, "Oops");
    LOG_F(WARNING, "Ummm...");
}

int
main()
{
    TEST_CASE(test_chomp);
    TEST_CASE(test_setloglevel);
    TEST_CASE(test_env_loglevel);

    TEST_CASE(emit_possibly_colorful_messages);

    TEST_RETURN;
}
