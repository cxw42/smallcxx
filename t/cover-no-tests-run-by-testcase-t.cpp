/// @file cover-no-tests-run-by-testcase-t.cpp
/// @brief Coverage for the no-tests-run-by-testcase situation.  This is an XFAIL.
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#include "smallcxx/test.hpp"

TEST_FILE

using namespace std;

void
no_assertions()
{
    LOG_F(INFO, "here");
}

int
main()
{
    TEST_CASE(no_assertions);
    TEST_RETURN;
}
