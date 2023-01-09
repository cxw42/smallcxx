/// @file t/globstari-ignore-control-t.cpp
/// @brief Test ignore-control functions
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021--2022 Christopher White

#include "smallcxx/test.hpp"

#include "testhelpers.hpp"

TEST_FILE

using namespace smallcxx;
using namespace std;
using smallcxx::glob::Path;

/// An IFileTree that produces a virtual filesystem including an eignore file.
class TestFileTreeIgnore: public IFileTree
{
public:
    std::vector< std::shared_ptr<Entry> >
    readDir(const Path& dirPath) override
    {
        std::vector< std::shared_ptr<Entry> > retval;

        if(dirPath == "/") {
            auto e = std::make_shared<Entry>(EntryType::File, "/.eignore");
            retval.push_back(e);
            e = std::make_shared<Entry>(EntryType::File, "/file");
            retval.push_back(e);
            e = std::make_shared<Entry>(EntryType::File, "/ignored");
            retval.push_back(e);

            // A force-unignored entry
            e = std::make_shared<Entry>(EntryType::File, "/ignored-never");
            e->neverIgnore = true;
            retval.push_back(e);
        }

        return retval;
    }

    Bytes
    readFile(const Path& path) override
    {
        if(path == "/.eignore") {
            return "ignored*\n";
        } else {
            return "";
        }
    }

    Path
    canonicalize(const Path& path) const override
    {
        return path;
    }
}; // class TestFileTreeIgnore

/// Test IProcessEntry::ignored()
static void
test_ignore_control()
{
    TestFileTreeIgnore fileTree;
    SaveEntries processEntry;
    reached();

    globstari(fileTree, processEntry, "/", {"*"});
    reached();

    // dir contents: `/`, `/file`, `/.eignore`, `/ignored-never'
    cmp_ok(processEntry.found.size(), ==, 4);

    auto found = processEntry.foundEntries["/file"];
    ok(!!found);
    ok(!found->ignored);
    found = processEntry.foundEntries["/ignored"];
    ok(!found);
    found = processEntry.foundEntries["/ignored-never"];
    ok(!!found);
    ok(found->ignored);
    ok(found->neverIgnore);

    found = processEntry.ignoredEntries["/file"];
    ok(!found);
    found = processEntry.ignoredEntries["/ignored"];
    ok(!!found);
    ok(found->ignored);
}

TEST_MAIN {
    TEST_CASE(test_ignore_control);
}
