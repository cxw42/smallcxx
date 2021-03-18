/// @file cover-no-tests-run-t.cpp
/// @brief Coverage for the no-tests-run case.  This is an XFAIL.
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#include "smallcxx/test.hpp"

TEST_FILE

using namespace std;

int
main()
{
    TEST_RETURN;
}
