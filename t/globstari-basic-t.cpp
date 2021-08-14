/// @file globstari-basic-t.cpp
/// @brief Basic tests of smallcxx/globstari.hpp classes
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#include "smallcxx/globstari.hpp"
#include "smallcxx/test.hpp"

TEST_FILE

using namespace smallcxx;
using namespace std;

/// A do-nothing concrete GlobstariBase subclass
class TestGlobstariBaseSanity: public GlobstariBase
{
    std::vector<Entry>
    readDir(const Path& dirPath) override
    {
        return {};
    }

    bool
    readFile(const Path& dirPath, const Path& name, Bytes& contents) override
    {
        return false;
    }

    ProcessStatus
    processEntry(const Entry& entry) override
    {
        return ProcessStatus::STOP;
    }
}; // class TestGlobstariBaseSanity

void
test_sanity()
{
    TestGlobstariBaseSanity s;
    reached();
}

int
main()
{
    TEST_CASE(test_sanity);
    TEST_RETURN;
}
