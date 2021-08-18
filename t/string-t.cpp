/// @file t/string.cpp
/// @brief Tests of smallcxx/string.hpp
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#define SMALLCXX_USE_CHOMP
#include "smallcxx/string.hpp"
#include "smallcxx/test.hpp"

TEST_FILE

using namespace std;
using smallcxx::trim;

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

void
test_trim()
{
    isstr(trim(""), "");
    isstr(trim(" a"), "a");
    isstr(trim("b "), "b");
    isstr(trim(" c "), "c");
}

int
main()
{
    TEST_CASE(test_chomp);
    TEST_CASE(test_trim);

    TEST_RETURN;
}
