/// @file globstari-globset-t.cpp
/// @brief Tests of smallcxx::glob::GlobSet
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#include "smallcxx/globstari.hpp"
#include "smallcxx/test.hpp"

TEST_FILE

using namespace smallcxx;
using namespace smallcxx::glob;
using namespace std;

void
test_empty()
{
    GlobSet gs;
    throws_with_msg(gs.contains("foo"), "not finalized");
    gs.finalize();
    ok(!gs.contains(""));
    ok(!gs.contains("foo"));
}

void
test_invalid()
{
    GlobSet gs;
    throws_with_msg(gs.addGlob(""), "empty glob");
}

void
test_exact_match()
{
    GlobSet gs;
    gs.addGlob("foo");
    gs.finalize();
    ok(gs.contains("foo"));
    ok(!gs.contains("dir/foo"));
    ok(!gs.contains("fooo"));
    ok(!gs.contains("f"));
    ok(!gs.contains("oo"));
    ok(!gs.contains(""));
    ok(!gs.contains("bar"));
}

void
test_extension()
{
    GlobSet gs;
    gs.addGlob("*.txt");
    gs.finalize();
    ok(gs.contains("foo.txt"));
    ok(gs.contains("fooo.txt"));
    ok(gs.contains(".txt"));    // Yes --- '*' can match 0 chars.
    ok(gs.contains(".txt.txt"));
    ok(!gs.contains("foo/bar.txt"));
    ok(!gs.contains(".txt."));
    ok(!gs.contains(".txt.bak"));
    ok(!gs.contains("foo"));
    ok(!gs.contains("fooo"));
    ok(!gs.contains(""));
    ok(!gs.contains("bar"));
}

void
test_namestart()
{
    GlobSet gs;
    gs.addGlob("file*");
    gs.finalize();
    ok(gs.contains("file"));
    ok(gs.contains("file1"));
    ok(gs.contains("filez"));
    ok(gs.contains("file.txt"));
    ok(gs.contains("file1.txt"));
    ok(gs.contains("filez.txt"));
    ok(!gs.contains(".file"));
    ok(!gs.contains(""));
}

void
test_globstar()
{
    GlobSet gs;
    gs.addGlob("**.txt");
    gs.finalize();
    ok(gs.contains("foo.txt"));
    ok(gs.contains("fooo.txt"));
    ok(gs.contains(".txt"));    // Yes --- '**' can match 0 chars.
    ok(gs.contains(".txt.txt"));
    ok(gs.contains("dir/foo.txt"));
    ok(!gs.contains(".txt."));
    ok(!gs.contains(".txt.bak"));
    ok(!gs.contains("foo"));
    ok(!gs.contains("fooo"));
    ok(!gs.contains(""));
    ok(!gs.contains("bar"));

    GlobSet gs2;
    gs2.addGlob("**/*.txt");
    gs2.finalize();
    ok(gs2.contains("/foo.txt"));
    ok(gs2.contains("/foo/bar.txt"));
    ok(gs2.contains("/foo/bar/bat.txt"));
    ok(!gs2.contains("/foo/bar/bat.txt.old"));
    ok(!gs2.contains("fooo.txt"));   // Need a slash in ec-style **/* globs
    ok(gs2.contains("/.txt"));        // '**', '*' can match 0 chars each.
    ok(gs2.contains("/.txt.txt"));
    ok(gs2.contains("dir/foo.txt"));
    ok(!gs2.contains(".txt."));
    ok(!gs2.contains(".txt.bak"));
    ok(!gs2.contains("foo"));
    ok(!gs2.contains("fooo"));
    ok(!gs2.contains(""));
    ok(!gs2.contains("bar"));
}

// === main ==============================================================

int
main()
{
    TEST_CASE(test_empty);
    TEST_CASE(test_invalid);
    TEST_CASE(test_exact_match);
    TEST_CASE(test_extension);
    TEST_CASE(test_namestart);
    TEST_CASE(test_globstar);

    TEST_RETURN;
}
