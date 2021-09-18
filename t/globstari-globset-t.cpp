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

    gs.finalize();
    throws_with_msg(gs.addGlob("*"), "finalized");
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
test_question()
{
    GlobSet gs;
    gs.addGlob("fo?.txt");
    gs.finalize();

    ok(gs.contains("foo.txt"));
    ok(!gs.contains("fo/.txt"));
}

void
test_brackets()
{
    GlobSet gs;
    gs.addGlob("fo[o].txt");
    gs.addGlob("fo[st].txt");
    gs.addGlob("fo[a-c].txt");
    gs.addGlob("fo[ef-g].txt");
    gs.finalize();

    ok(gs.contains("foa.txt"));
    ok(gs.contains("fob.txt"));
    ok(gs.contains("foc.txt"));
    ok(!gs.contains("fod.txt"));
    ok(gs.contains("foe.txt"));
    ok(gs.contains("fof.txt"));
    ok(gs.contains("fog.txt"));
    ok(!gs.contains("foh.txt"));
    ok(!gs.contains("foi.txt"));
    ok(!gs.contains("foj.txt"));
    ok(!gs.contains("fok.txt"));
    ok(!gs.contains("fol.txt"));
    ok(!gs.contains("fom.txt"));
    ok(!gs.contains("fon.txt"));
    ok(gs.contains("foo.txt"));
    ok(!gs.contains("fop.txt"));
    ok(!gs.contains("foq.txt"));
    ok(!gs.contains("for.txt"));
    ok(gs.contains("fos.txt"));
    ok(gs.contains("fot.txt"));
    ok(!gs.contains("fou.txt"));
    ok(!gs.contains("fov.txt"));
    ok(!gs.contains("fow.txt"));
    ok(!gs.contains("fox.txt"));
    ok(!gs.contains("foy.txt"));
    ok(!gs.contains("foz.txt"));
    ok(!gs.contains("fo/.txt"));
}

void
test_braces()
{
    {
        GlobSet gs;
        gs.addGlob("*.{txt,pl}");
        gs.finalize();

        ok(gs.contains(".txt"));
        ok(gs.contains("foo.txt"));
        ok(gs.contains(".pl"));
        ok(gs.contains("foo.pl"));
        ok(!gs.contains("foo.txt.bak"));
    }

    {
        // Multiple numeric ranges in one GlobSet.  smallcxx handles this case
        // differently than editorconfig-core-c does.
        GlobSet gs;
        gs.addGlob("{1..10}");
        gs.addGlob("{100..109}");
        gs.finalize();

        ok(!gs.contains(""));
        ok(!gs.contains("foo"));
        ok(!gs.contains("0"));
        ok(gs.contains("1"));
        ok(gs.contains("10"));
        ok(!gs.contains("11"));
        ok(!gs.contains("20"));
        ok(!gs.contains("20"));
        ok(!gs.contains("99"));
        ok(gs.contains("100"));
        ok(gs.contains("109"));
        ok(!gs.contains("110"));
    }

    {
        // The same numeric range twice in one GlobSet.
        GlobSet gs;
        gs.addGlob("{1..10}");
        gs.addGlob("{1..10}");
        gs.finalize();

        ok(!gs.contains(""));
        ok(!gs.contains("foo"));
        ok(!gs.contains("0"));
        ok(gs.contains("1"));
        ok(gs.contains("10"));
        ok(!gs.contains("11"));
    }
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

void
test_utf8()
{
    GlobSet gs;
    gs.addGlob("コンニチハ*");
    gs.finalize();

    ok(gs.contains("コンニチハ"));
    ok(gs.contains("コンニチハ to you as well!"));
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

    TEST_CASE(test_question);
    TEST_CASE(test_brackets);
    TEST_CASE(test_braces);
    TEST_CASE(test_globstar);
    TEST_CASE(test_utf8);

    TEST_RETURN;
}
// vi: set fdm=marker fenc=utf-8: //
