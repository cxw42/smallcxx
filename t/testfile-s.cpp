/// @file t/testfile-s.cpp
/// @brief A sample test file.  Used to test logging functions in test code.
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2022 Christopher White

#include "smallcxx/logging.hpp"
#include "smallcxx/test.hpp"

TEST_FILE;

void
test_dummy()
{
    LOG_F(LOG, "avocado");
    cmp_ok(1 + 1, ==, 2);
}

TEST_MAIN {
    TEST_CASE(test_dummy);
}
