/// @file meta-t.cpp
/// @brief Tests of test.hpp itself!
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White
///
/// In production, I would make test.hpp more flexible so I could test
/// failures in this file as well as successes.

#include "smallcxx/test.hpp"

using namespace std;

TEST_FILE

void
test_ok()
{
    ok(true);
    ok(1 == 1);
    reached();
}

void
test_cmp_ok()
{
    cmp_ok(0, ==, 0);
    cmp_ok(1, !=, 2);
    cmp_ok(1, <, 2);
    cmp_ok(4, >, 3);
    cmp_ok(5, <=, 5);
    cmp_ok(5, <=, 6);
    cmp_ok(7, >=, 7);
    cmp_ok(8, >=, 7);
}

void
test_isstr()
{
    isstr("foo", "foo");
    string bar("bar");
    isstr(bar, bar);
    isstr(bar, string("bar"));
    isstr(bar, string("bar").c_str());
}

void
test_exceptions()
{
    throws_with_msg(
        throw runtime_error("yowza there"),
        "yowza"
    );

    does_not_throw( (void)1 );
}

int
main()
{
    TEST_CASE(test_ok);
    TEST_CASE(test_cmp_ok);
    TEST_CASE(test_isstr);
    TEST_CASE(test_exceptions);
    TEST_RETURN;
}
