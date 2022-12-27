/// @file t/globstari-userdata-t.cpp
/// @brief Test smallcxx::Entry::userdata
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021--2022 Christopher White

#include <set>
#include <utility>

#include "smallcxx/globstari.hpp"
#include "smallcxx/test.hpp"

TEST_FILE

using namespace smallcxx;
using namespace std;
using smallcxx::glob::Path;

/// An IFileTree that produces a virtual filesystem.
/// The filesystem includes exactly directory `/` and file `/file`.
class TestFileTreeUserdata: public IFileTree
{
public:
    std::vector< std::shared_ptr<Entry> >
    readDir(const Path& dirPath) override
    {
        std::vector< std::shared_ptr<Entry> > retval;

        if(dirPath == "/") {
            auto e = std::make_shared<Entry>(EntryType::File, "/file");
            e->userdata = 42;
            retval.push_back(std::move(e));
        }

        return retval;
    }

    Bytes
    readFile(const Path& path) override
    {
        return "";
    }

    Path
    canonicalize(const Path& path) const
    {
        return path;
    }
}; // class TestFileTreeUserdata

/// Save files traversed.
class SaveEntries: public IProcessEntry
{
public:
    std::vector< std::shared_ptr<Entry> > found;

    IProcessEntry::Status
    operator()(const  std::shared_ptr<Entry>& entry) override
    {
        LOG_F(TRACE, "Found %s", entry->canonPath.c_str());
        if(entry->ty == EntryType::File) {
            found.push_back(entry);
        }
        return IProcessEntry::Status::Continue;
    }

}; // SaveEntries

static void
test_userdata()
{
    TestFileTreeUserdata fileTree;
    SaveEntries processEntry;
    reached();

    globstari(fileTree, processEntry, "/", {"*"});
    reached();

    cmp_ok(processEntry.found.size(), ==, 1);
    cmp_ok(processEntry.found[0]->userdata, ==, 42);
}

TEST_MAIN {
    TEST_CASE(test_userdata);
}
