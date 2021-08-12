/// @file test.hpp
/// @brief A testing framework - declarations
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White
///
/// Example test file:
/// ```
/// #include "test.hpp"
/// TEST_FILE
///
/// void test_something()
/// {
///     ok(true);
///     cmp_ok(1, ==, 1);
///     isstr("foo", "foo");
/// }
///
/// int main()
/// {
///     TEST_CASE(test_something);
///     TEST_RETURN;
/// }
/// ```
///
/// This file is inspired by (but not copied from):
/// - Perl's [Test::More](https://metacpan.org/pod/Test::More)
/// - Perl's [Test::Exception](https://metacpan.org/pod/Test::Exception)
/// - [GLib Testing](https://developer.gnome.org/glib/stable/glib-Testing.html)

#ifndef TEST_HPP_
#define TEST_HPP_

#include <inttypes.h>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <string.h>
#include <smallcxx/logging.hpp>

// === Logging control ===================================================

#ifndef TEST_DETAIL_ENV_VAR_NAME

/// The name of an environment variable used for detailed log-level control.
/// To support detailed control, define this to a string before including
/// this file.  `$V` is always supported.
#define TEST_DETAIL_ENV_VAR_NAME

#endif

// === Constants and functions ===========================================

/// Exit codes recognized by Automake's parallel (non-TAP) test harness
enum TestExitCode {
    TEST_PASS = 0,
    TEST_FAIL = 1,
    TEST_SKIP = 77,
    TEST_STOP_TESTING = 99,  ///< bail out --- testing can't continue
};

/// Make a filename relative to $top_srcdir/t
std::string testDataFilename(const char *filename);

// === Macros ============================================================

/// Create global state for a test file.
///
/// This macro:
/// - creates the TEST_failures and TEST_successes variables that will hold the
///   number of failures during this run.
/// - defines constants used by reached() and unreached() to provide more
///   readable assertion-failure messages
/// - sets the verbosity level based on @c $V.
#define TEST_FILE \
    static unsigned int TEST_failures = 0; \
    static unsigned int TEST_successes = 0; \
    static const bool reached_this_line_as_expected = true; \
    static const bool reached_this_line_unexpectedly = false; \
    static const bool _initialized_log = ([](){ \
            setVerbosityFromEnvironment(TEST_DETAIL_ENV_VAR_NAME); \
            LOG_F_DOMAIN(" test", LOG, "Initialized logging"); \
            return true || reached_this_line_as_expected || \
                reached_this_line_unexpectedly; \
    })();
// The "||reached..." in the above macro are to remove "unused variable"
// warnings from clang

/// Normal test return, pass/fail.
#define TEST_RETURN \
    do { \
        if(TEST_failures) { \
            LOG_F_DOMAIN(" test", ERROR, "%u test%s failed", TEST_failures, \
                    TEST_failures > 1 ? "s" : ""); \
            return TEST_FAIL; \
        } else if(TEST_successes == 0) { \
            LOG_F_DOMAIN(" test", ERROR, "No tests ran"); \
            return TEST_FAIL; \
        } else { \
            LOG_F_DOMAIN(" test", INFO, "All tests passed"); \
            return TEST_PASS; \
        } \
    } while(0)

/// Abort this test and return TEST_SKIP.  Logs an Info message.
#define TEST_SKIP_ALL(format, ...) \
    do { \
        LOG_F_DOMAIN(" test", INFO, "SKIP all: " format, ## __VA_ARGS__); \
        return TEST_SKIP; \
    } while(0);

/// Abort and fail this test.  Logs an Error message.
#define TEST_ABORT(format, ...) \
    do { \
        LOG_F_DOMAIN(" test", ERROR, "ABORT: " format, ## __VA_ARGS__); \
        return TEST_FAIL; \
    } while(0);

/// Abort this test and return TEST_STOP_TESTING.  Logs an Error message.
#define TEST_BAIL_OUT(format, ...) \
    do { \
        LOG_F_DOMAIN(" test", ERROR, "Bail out!  " format, ## __VA_ARGS__); \
        return TEST_STOP_TESTING; \
    } while(0);

/// @name Specifying test cases
/// @{

/// Run the given void function, with logging around it.
/// "NOTRY" because exceptions propagate out.
/// Does not include any test assertions.
#define TEST_CASE_NOTRY(fn) \
    do { \
        LOG_F_DOMAIN(" test", LOG, "=> Starting test %s", #fn); \
        fn(); \
        LOG_F_DOMAIN(" test", LOG, "<= Finished test %s", #fn); \
    } while(0)

/// Run the given void function, with logging around it, inside a
/// `try` block.  Exceptions are logged and count as test failures,
/// and exiting @p fn with no exceptions counts as a successful test.
/// @param[in]  fn - The name of the function to run (or anything else
///     for which `fn();` is valid).
#define TEST_CASE(fn) \
    do { \
        try { \
            LOG_F_DOMAIN(" test", LOG, "=> Starting test %s", #fn); \
            fn(); \
            LOG_F_DOMAIN(" test", LOG, "<= Finished test %s", #fn); \
            reached(); \
        } catch(std::exception& e) { \
            LOG_F_DOMAIN(" test", ERROR, "Caught exception: %s", e.what()); \
            unreached(); \
        } catch(...) { \
            LOG_F_DOMAIN(" test", ERROR, "Caught unexpected exception"); \
            unreached(); \
        } \
    } while(0)

// todo in the future: add setup/teardown

/// @}
// end specifying test cases

/// @name Test assertions
/// @brief  Functions that adjust the test failure count.  Unlike assert(),
///         they do NOT terminate execution of the test file.
/// @note   In production, I would also provide optional custom messages
///         associated with assertions.
/// @{

/// Master assertion routine.
/// If @p cond is true, increment @p successes.  Otherwise, increment
/// @p failures and log a test-failure message.
///
/// @param[in,out]  failures    Incremented if @p cond is false
/// @param[in,out]  successes   Incremented if @p cond is true
/// @param[in]      file        __FILE__
/// @param[in]      line        __LINE__
/// @param[in]      function    __func__
/// @param[in]      cond        True if the test passed, false otherwise
/// @param[in]      format      Failure message format string
void test_assert(unsigned int& failures, unsigned int& successes,
                 const char *file, int line,
                 const char *function,
                 bool cond, const char *format, ...)
__attribute__(( format(printf, 7, 8) ));

/// Assert that @p cond is true.
#define ok(cond, ...) \
    do { \
        const bool _result = (cond); \
        test_assert(TEST_failures, TEST_successes, __FILE__, __LINE__, __func__, \
            _result, "%s was false (expected true)", #cond); \
    } while(0)

/// Compare numeric values.
/// @warning This only works for values convertible to intmax_t.
#define cmp_ok(got, op, expected, ...) \
    do { \
        const auto _got = (got); \
        const auto _expected = (expected); \
        const bool _result = (_got op _expected); \
        test_assert(TEST_failures, TEST_successes, __FILE__, __LINE__, __func__, _result, \
                "!(%s %s %s): got %" PRIdMAX ", expected %" PRIdMAX, \
                (#got), (#op), (#expected), (intmax_t)_got, (intmax_t)_expected); \
    } while(0)

/// Compare string values for equality.
///
/// @pre @parblock
/// A std::string can be initialized from each of @p got and @p expected.
/// This macro copies its arguments into std::string so that it's OK to pass
/// temporary std::string instances.  E.g.,
/// ```
/// isstr("foo", (STR_OF << "foo"))
/// ```
/// works fine.  Note, however, that
/// ```
/// isstr("foo", (STR_OF << "foo").c_str())
/// ```
/// does not work in my tests, because the StringFormatter() instance gets
/// destroyed before the strcmp().
/// @endparblock
#define isstr(got, expected) \
    do { \
        const std::string _got(got); \
        const std::string _expected(expected); \
        const bool _result = (strcmp(_got.c_str(), _expected.c_str()) == 0); \
        test_assert(TEST_failures, TEST_successes, __FILE__, __LINE__, __func__, _result, \
                "%s != %s: got `%s', expected `%s'", \
                (#got), (#expected), (_got.c_str()), (_expected.c_str())); \
    } while(0)

/// Assert that a given statement threw a std::exception containing a
/// given substring.
/// @param[in]  stmt        A statement, WITHOUT the trailing semicolon
/// @param[in]  expected    A string expected as a substring of the
///                         std::exception::what() of the thrown exception.
#define throws_with_msg(stmt, expected) \
    do { \
        const string _expected(expected); \
        try { \
            stmt; \
            unreached(); \
            LOG_F_DOMAIN(" test", ERROR, "Did not throw, but expected to"); \
        } catch (std::exception& e) { \
            reached(); \
            LOG_F_DOMAIN(" test", LOG, "Got exception `%s'", e.what()); \
            bool didmatch = (strstr(e.what(), _expected.c_str()) != nullptr); \
            ok(didmatch); \
            if(!didmatch) { \
                LOG_F_DOMAIN(" test", ERROR, \
                        "Exception `%s` did not include expected text `%s`", \
                        e.what(), _expected.c_str()); \
            } \
        } catch(...) { \
            unreached(); \
            LOG_F_DOMAIN(" test", WARNING, "Got exception, but not one I expected"); \
        } \
    } while(0)

/// Assert that a given statement threw some exception
/// @param[in]  stmt        A statement, WITHOUT the trailing semicolon
#define throws_ok(stmt) \
    do { \
        try { \
            stmt; \
            unreached(); \
            LOG_F_DOMAIN(" test", ERROR, "Did not throw, but expected to"); \
        } catch (std::exception& e) { \
            LOG_F_DOMAIN(" test", LOG, "Got exception as expected (`%s')", e.what()); \
            reached(); \
        } catch(...) { \
            LOG_F_DOMAIN(" test", LOG, "Got exception as expected"); \
            reached(); \
        } \
    } while(0)

/// Assert that a given statement did not throw
/// @param[in]  stmt        A statement, WITHOUT the trailing semicolon
#define does_not_throw(stmt) \
    do { \
        try { \
            stmt; \
            reached(); \
            LOG_F_DOMAIN(" test", LOG, "Did not throw, as expected"); \
        } catch (std::exception& e) { \
            unreached(); \
            LOG_F_DOMAIN(" test", ERROR, "Got exception `%s'", e.what()); \
        } catch(...) { \
            unreached(); \
            LOG_F_DOMAIN(" test", ERROR, "Got unknown exception"); \
        } \
    } while(0)

/// Assert that we got here and wanted to.
#define reached() \
    ok(reached_this_line_as_expected)

/// Assert false because we got here but didn't want to!
#define unreached() \
    ok(reached_this_line_unexpectedly)

/// @}
// (end of test assertions)

#endif // TEST_HPP_
