/// @file cover-test-failures-t.cpp
/// @brief Coverage for things that fail.  This is an XFAIL.
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#include "smallcxx/test.hpp"

TEST_FILE

using namespace std;

int
main()
{
    unreached();
    TEST_RETURN;
}
