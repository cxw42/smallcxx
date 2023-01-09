/// @file globstari-basic-t.cpp
/// @brief Basic tests of smallcxx/globstari.hpp classes
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021--2022 Christopher White
/// SPDX-License-Identifier: BSD-3-Clause

#include <set>

#include "smallcxx/globstari.hpp"
#include "smallcxx/test.hpp"

#include "testhelpers.hpp"

TEST_FILE

using namespace smallcxx;
using namespace std;
using smallcxx::glob::Path;

/// A do-nothing concrete IFileTree subclass
class TestFileTreeSanity: public IFileTree
{
public:
    // LCOV_EXCL_START - since TestProcessEntrySanity returns Stop on the
    // root dir, readDir() is never called.  Since there are no entries,
    // readFile is never called.
    std::vector< std::shared_ptr<Entry> >
    readDir(const Path& dirPath) override
    {
        return {};
    }

    Bytes
    readFile(const Path& path) override
    {
        return "";
    }
    // LCOV_EXCL_STOP

    Path
    canonicalize(const Path& path) const override
    {
        return path;
    }
}; // class TestFileTreeSanity

/// A do-nothing concrete IProcessEntry subclass
class TestProcessEntrySanity: public IProcessEntry
{
public:
    IProcessEntry::Status
    operator()(const  std::shared_ptr<Entry>& entry) override
    {
        return IProcessEntry::Status::Stop;
    }
};

static void
test_sanity()
{
    TestFileTreeSanity fileTree;
    TestProcessEntrySanity processEntry;
    reached();

    globstari(fileTree, processEntry, "/", {"*"});
    reached();

    // FileTree and ProcessEntry instances can, in general, be used more than once
    globstari(fileTree, processEntry, "/", {"*"});
    reached();

    // Empty needle is not allowed
    throws_with_msg(
        globstari(fileTree, processEntry, "/", {}),
        "needle.empty"
    );
}

/// @name Tests of disk globbing
/// @{

/// Tests using the contents of `t/globstari-basic-disk`.
static void
test_disk()
{
    const glob::Path basepath{SRCDIR "/globstari-basic-disk"};
    LOG_F(INFO, "Base path of test tree is %s", basepath.c_str());
    DiskFileTree fileTree;

    {
        // matches none
        SaveEntries saveEntries;
        globstari(fileTree, saveEntries, basepath, {"NONEXISTENT"});
        cmp_ok(saveEntries.found.size(), ==, 0);
        cmp_ok(saveEntries.ignoredPaths.size(), ==, 0);
    }

    {
        // matches one
        SaveEntries saveEntries;
        globstari(fileTree, saveEntries, basepath, {"noex*"});
        compare_sequence(saveEntries.found, {"globstari-basic-disk/noext"}, __func__,
                         __LINE__);
        cmp_ok(saveEntries.ignoredPaths.size(), ==, 0);
    }

    {
        // matches two
        SaveEntries saveEntries;
        globstari(fileTree, saveEntries, basepath, {"*.txt"});
        compare_sequence(saveEntries.found, {"/text.txt", "/text2.txt"}, __func__,
                         __LINE__);
        cmp_ok(saveEntries.ignoredPaths.size(), ==, 0);
    }

    {
        // with exclusions
        SaveEntries saveEntries;
        globstari(fileTree, saveEntries, basepath, {"*.txt", "!text.txt"});
        compare_sequence(saveEntries.found, {"/text2.txt"}, __func__, __LINE__);
        cmp_ok(saveEntries.ignoredPaths.size(), ==, 0);
    }

    {
        // matches one in a subdir
        SaveEntries saveEntries;
        globstari(fileTree, saveEntries, basepath, {"somef*"});
        compare_sequence(saveEntries.found, {"globstari-basic-disk/subdir/somefile"},
                         __func__,
                         __LINE__);
        cmp_ok(saveEntries.ignoredPaths.size(), ==, 0);
    }

    {
        // matches all
        SaveEntries saveEntries;
        globstari(fileTree, saveEntries, basepath, {"*"});
        compare_sequence(saveEntries.found, {"/binary.bin", "/noext", "/subdir", "/subdir/somefile", "text.txt", "text2.txt"},
                         __func__, __LINE__);
        cmp_ok(saveEntries.ignoredPaths.size(), ==, 0);
    }

} // test_disk()

/// Tests using the contents of `t/globstari-basic-disk-ignores`.
static void
test_disk_ignores()
{
    const glob::Path basepath{SRCDIR "/globstari-basic-disk-ignores"};
    LOG_F(INFO, "Base path of test tree is %s", basepath.c_str());
    DiskFileTree fileTree;

    {
        // matches two; neither is ignored (sanity check)
        SaveEntries saveEntries;
        globstari(fileTree, saveEntries, basepath, {"*.txt"});
        compare_sequence(saveEntries.found, {"/text.txt", "/text2.txt"}, __func__,
                         __LINE__);
        cmp_ok(saveEntries.ignoredPaths.size(), ==, 5);
        // Five ignored paths:
        //  - ignored by t/globstari-basic-disk-ignores/.eignore:
        //      - file#4
        //      - ignored.1
        //      - ignored.2
        //      - dir/subdir/s3dir/ignored.in-s3dir
        //  - ignored by t/globstari-basic-disk-ignores/dir/subdir/s2dir/eignore:
        //      - dir/subdir/s2dir/s3dir/subignored
    }

    {
        // with ignores
        SaveEntries saveEntries;
        globstari(fileTree, saveEntries, basepath, {"ignored*"});
        compare_sequence(saveEntries.found, {"/ignored.not-actually"}, __func__,
                         __LINE__);
        cmp_ok(saveEntries.ignoredPaths.size(), ==, 5);
    }

    {
        // with ignores in subdirs
        SaveEntries saveEntries;
        globstari(fileTree, saveEntries, basepath, {"*ignored*"});
        compare_sequence(saveEntries.found, {
            "/dir/subdir/s2dir/s3dir/notignored",
            "/dir/subignored-not-actually",
            "/ignored.not-actually",
        }, __func__, __LINE__);
        cmp_ok(saveEntries.ignoredPaths.size(), ==, 5);
    }

    {
        // Make sure `#` in .eignore doesn't count as a glob
        SaveEntries saveEntries;
        globstari(fileTree, saveEntries, basepath, {"#"});
        compare_sequence(saveEntries.found, {"/#"}, __func__, __LINE__);
        cmp_ok(saveEntries.ignoredPaths.size(), ==, 5);
    }

    {
        // Escaped `#` in patterns
        SaveEntries saveEntries;
        globstari(fileTree, saveEntries, basepath, {"file*"});
        compare_sequence(saveEntries.found,
        {"/dir/file#3", "/file#1", "/file#2", "/file#3"},
        __func__, __LINE__);
        cmp_ok(saveEntries.ignoredPaths.size(), ==, 5);
    }

    {
        // Patterns anchored at a level
        SaveEntries saveEntries;
        globstari(fileTree, saveEntries, basepath, {"/file*"});
        compare_sequence(saveEntries.found, {"/file#1", "/file#2", "/file#3"},
                         __func__, __LINE__);
        cmp_ok(saveEntries.ignoredPaths.size(), ==, 5);
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
