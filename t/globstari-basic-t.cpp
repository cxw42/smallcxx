/// @file globstari-basic-t.cpp
/// @brief Basic tests of smallcxx/globstari.hpp classes
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#include <set>

#include "smallcxx/globstari.hpp"
#include "smallcxx/test.hpp"

TEST_FILE

using namespace smallcxx;
using namespace std;
using smallcxx::glob::Path;

/// A do-nothing concrete GlobstariBase subclass
class TestGlobstariBaseSanity: public GlobstariBase
{
    std::vector<Entry>
    readDir(const Path& dirPath) override
    {
        return {};
    }

    Bytes
    readFile(const Path& path) override
    {
        return "";
    }

    ProcessStatus
    processEntry(const Entry& entry) override
    {
        return ProcessStatus::Stop;
    }

    Path
    canonicalize(const Path& path) const
    {
        return path;
    }
}; // class TestGlobstariBaseSanity

static void
test_sanity()
{
    TestGlobstariBaseSanity s;
    reached();

    s.traverse("/", {"*"});
    reached();

    // A single GlobstariBase instance can be used more than once
    s.traverse("/", {"*"});
    reached();
}

/// @name Tests of disk globbing
/// @{

/// Test if @p got matches @p expectedPaths.
static void
compare_sequence(const set<glob::Path>& got,
                 const vector<glob::Path>& expectedPaths,
                 const char *func, const size_t line)
{
    // INFO log level so it will appear by default --- otherwise we don't know
    // which test failed.
    LOG_F(INFO, "Checking %s():%zu", func, line);

    cmp_ok(got.size(), ==, expectedPaths.size());
    size_t idx = 0;
    for(auto it = got.cbegin();
            (it != got.cend()) && (idx < expectedPaths.size());
            ++it, ++idx) {
        const auto& got = *it;
        const auto& expected = expectedPaths[idx];
        const auto where = got.rfind(expected);

        LOG_F(SNOOP, "got [%s], expected [%s]", got.c_str(), expected.c_str());
        cmp_ok(where, !=, got.npos);
    }
} // compare_sequence()

/// Save filenames traversed.
/// For testing globbing on disk.
struct DiskTraverser: public GlobstariDisk {
    using GlobstariDisk::GlobstariDisk;

    set<glob::Path> found;

    virtual ProcessStatus
    processEntry(const Entry& entry)
    {
        LOG_F(TRACE, "Found %s", entry.canonPath.c_str());
        found.insert(entry.canonPath);
        return ProcessStatus::Continue;
    }

}; // DiskTraverser

/// Tests using the contents of `t/globstari-basic-disk`.
static void
test_disk()
{
    const glob::Path basepath{SRCDIR "/globstari-basic-disk"};
    LOG_F(INFO, "Base path of test tree is %s", basepath.c_str());

    {
        // matches none
        DiskTraverser d;
        d.traverse(basepath, {"NONEXISTENT"});
        cmp_ok(d.found.size(), ==, 0);
    }

    {
        // matches one
        DiskTraverser d;
        d.traverse(basepath, {"noex*"});
        compare_sequence(d.found, {"globstari-basic-disk/noext"}, __func__, __LINE__);
    }

    {
        // matches two
        DiskTraverser d;
        d.traverse(basepath, {"*.txt"});
        compare_sequence(d.found, {"/text.txt", "/text2.txt"}, __func__, __LINE__);
    }

    {
        // with exclusions
        DiskTraverser d;
        d.traverse(basepath, {"*.txt", "!text.txt"});
        compare_sequence(d.found, {"/text2.txt"}, __func__, __LINE__);
    }

} // test_disk()

/// Tests using the contents of `t/globstari-basic-disk-ignores`.
static void
test_disk_ignores()
{
    const glob::Path basepath{SRCDIR "/globstari-basic-disk-ignores"};
    LOG_F(INFO, "Base path of test tree is %s", basepath.c_str());

    {
        // matches two; neither is ignored (sanity check)
        DiskTraverser d;
        d.traverse(basepath, {"*.txt"});
        compare_sequence(d.found, {"/text.txt", "/text2.txt"}, __func__, __LINE__);
    }

    {
        // with ignores
        DiskTraverser d;
        d.traverse(basepath, {"ignored*"});
        compare_sequence(d.found, {"/ignored.not-actually"}, __func__, __LINE__);
    }

    {
        // with ignores in subdirs
        DiskTraverser d;
        d.traverse(basepath, {"*ignored*"});
        compare_sequence(d.found, {
            "/dir/subdir/s2dir/s3dir/notignored",
            "/dir/subignored-not-actually",
            "/ignored.not-actually",
        }, __func__, __LINE__);
    }

    {
        // Make sure `#` in .eignore doesn't count as a glob
        DiskTraverser d;
        d.traverse(basepath, {"#"});
        compare_sequence(d.found, {"/#"}, __func__, __LINE__);
    }

    {
        // Escaped `#` in patterns
        DiskTraverser d;
        d.traverse(basepath, {"file*"});
        compare_sequence(d.found,
        {"/dir/file#3", "/file#1", "/file#2", "/file#3"},
        __func__, __LINE__);
    }

    {
        // Patterns anchored at a level
        DiskTraverser d;
        d.traverse(basepath, {"/file*"});
        compare_sequence(d.found, {"/file#1", "/file#2", "/file#3"},
                         __func__, __LINE__);
    }
} // test_disk_ignores()

/// @}

TEST_MAIN {
    LOG_F(INFO, "SRCDIR [%s], MY_PATH [%s], argv[0] [%s]",
          SRCDIR, MY_PATH.c_str(), argv[0]);
    TEST_CASE(test_sanity);
    TEST_CASE(test_disk);
    TEST_CASE(test_disk_ignores);
}
