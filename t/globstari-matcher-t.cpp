/// @file globstari-matcher-t.cpp
/// @brief Tests of smallcxx::glob::Matcher
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White
/// @copyright Uses some code from editorconfig-core-test, licensed BSD.
///     See below for the editorconfig-core-test copyright statement.
///
/// This file includes tests from the EditorConfig core test suite.  Those
/// tests are here rather than in t/globstari-globstar-t.cpp since the
/// full EditorConfig glob matching is in the Matcher in smallcxx.

#include "smallcxx/globstari.hpp"
#include "smallcxx/test.hpp"

TEST_FILE

using namespace smallcxx;
using namespace smallcxx::glob;
using namespace std;

// Basic tests {{{1

void
test_empty()
{
    Matcher m;
    ok(m.ready());
    m.finalize();
    ok(m.ready());
    ok(!m.contains(""));
    cmp_ok(m.check(""), ==, PathCheckResult::Unknown );
}

void
test_invalid()
{
    {
        Matcher m;
        throws_with_msg(m.addGlob(""), "empty glob");
    }

    {
        Matcher m;
        m.addGlob("*");
        m.finalize();
        ok(m.ready());
        ok(!m.contains(""));
        throws_with_msg(m.contains("relative-path"), "must be absolute");
    }
}

void
test_not_finalized()
{
    Matcher m;
    m.addGlob("foo");
    ok(!m.ready());
    throws_with_msg(m.contains("foo"), "not ready");
    throws_with_msg(m.check("foo"), "not ready");
    m.finalize();
    ok(m.ready());
    ok(!m.contains(""));
}

/// https://github.com/editorconfig/editorconfig/issues/455
void
test_ec455()
{
    {
        Matcher m({"[[a-b]"}, "/");
        ok(m.contains("/["));
        ok(m.contains("/a"));
        ok(m.contains("/b"));
        ok(!m.contains("/c"));
        ok(!m.contains("/\\"));
        ok(!m.contains("/]"));
    }

    {
        Matcher m({R"([a\-c])"}, "/");
        ok(m.contains("/a"));
        ok(m.contains("/-"));
        ok(m.contains("/c"));
        ok(!m.contains("/b"));  // the - isn't a range
    }

    {
        Matcher m({"-"}, "/");
        ok(!m.contains("/a"));
        ok(m.contains("/-"));
    }

    {
        Matcher m({"}{"}, "/");
        ok(m.contains("/}{"));
        ok(!m.contains("/"));
    }

    {
        Matcher m({"\\"}, "/");
        ok(m.contains("/\\"));
        ok(!m.contains("/"));
    }
}

/// Tests of special characters in directory names
void
test_specialchar_dirname()
{
    // Tests are in order of ec_special_chars

    {
        Matcher m({"*.txt"}, "/?/");
        ok(!m.contains("/?"));
        ok(!m.contains("/x.txt"));
        ok(m.contains("/?/x.txt"));
    }

    {
        Matcher m({"*.txt"}, "/[/");
        ok(!m.contains("/["));
        ok(!m.contains("/x.txt"));
        ok(m.contains("/[/x.txt"));
    }

    {
        Matcher m({"*.txt"}, "/]/");
        ok(!m.contains("/]"));
        ok(!m.contains("/x.txt"));
        ok(m.contains("/]/x.txt"));
    }

    // TODO what about backslashes in dirnames?

    {
        Matcher m({"*.txt"}, "/*/");
        ok(!m.contains("/*"));
        ok(!m.contains("/x.txt"));
        ok(m.contains("/*/x.txt"));
    }

    {
        Matcher m({"*.txt"}, "/-/");
        ok(!m.contains("/-"));
        ok(!m.contains("/x.txt"));
        ok(m.contains("/-/x.txt"));
    }

    {
        Matcher m({"*.txt"}, "/{/");
        ok(!m.contains("/{"));
        ok(!m.contains("/x.txt"));
        ok(m.contains("/{/x.txt"));
    }

    {
        Matcher m({"*.txt"}, "/}/");
        ok(!m.contains("/}"));
        ok(!m.contains("/x.txt"));
        ok(m.contains("/}/x.txt"));
    }

    {
        Matcher m({"*.txt"}, "/,/");
        ok(!m.contains("/,"));
        ok(!m.contains("/x.txt"));
        ok(m.contains("/,/x.txt"));
    }

}

// }}}1

/// @name Tests of addGlob(glob)
/// @{
// addGlob(glob) {{{1

void
test_exact_match()
{
    Matcher m;
    m.addGlob("/foo");
    m.finalize();
    ok(m.contains("/foo"));
    cmp_ok(m.check("/foo"), ==, PathCheckResult::Included);
    ok(!m.contains("/fooo"));
    cmp_ok(m.check("/fooo"), ==, PathCheckResult::Unknown);
    ok(!m.contains("/f"));
    ok(!m.contains("/oo"));
    ok(!m.contains(""));
    ok(!m.contains("/bar"));
}

void
test_extension()
{
    Matcher m;
    m.addGlob("*.txt", "/");
    m.finalize();
    ok(m.contains("/foo.txt"));
    cmp_ok(m.check("/foo.txt"), ==, PathCheckResult::Included);
    ok(m.contains("/fooo.txt"));
    ok(m.contains("/.txt"));    // Yes --- '*' can match 0 chars.
    ok(m.contains("/.txt.txt"));
    ok(!m.contains("/.txt."));
    cmp_ok(m.check("/.txt."), ==, PathCheckResult::Unknown);
    ok(!m.contains("/.txt.bak"));
    ok(!m.contains("/foo"));
    ok(!m.contains("/fooo"));
    ok(!m.contains(""));
    ok(!m.contains("/bar"));
}

void
test_extension_negpos()
{
    Matcher m;
    m.addGlob("!*.txt", "/");
    m.addGlob("*.txt", "/");
    m.finalize();
    ok(!m.contains(""));

    ok(m.contains("/foo.txt"));
    cmp_ok(m.check("/foo.txt"), ==, PathCheckResult::Included);
    cmp_ok(m.check("/bar"), ==, PathCheckResult::Unknown);
}

void
test_extension_posneg()
{
    Matcher m;
    m.addGlob("*.txt", "/");
    m.addGlob("!*.txt", "/");
    m.finalize();
    ok(!m.contains(""));

    ok(!m.contains("/foo.txt"));
    cmp_ok(m.check("/foo.txt"), ==, PathCheckResult::Excluded);
    cmp_ok(m.check("/bar"), ==, PathCheckResult::Unknown);
}

void
test_namestart()
{
    Matcher m;
    m.addGlob("/file*");
    m.finalize();
    ok(m.contains("/file"));
    ok(m.contains("/file1"));
    ok(m.contains("/filez"));
    ok(m.contains("/file.txt"));
    ok(m.contains("/file1.txt"));
    ok(m.contains("/filez.txt"));
    ok(!m.contains("/.file"));
    ok(!m.contains(""));
}

// }}}1
/// @}

/// @name Tests of addGlob(glob, path)
/// @{
// addGlob(glob, path) {{{1

void
test_path_namestart()
{
    Matcher m;
    m.addGlob("file*", "/");
    m.finalize();
    ok(m.contains("/file"));
    ok(m.contains("/file1"));
    ok(m.contains("/filez"));
    ok(m.contains("/file.txt"));
    ok(m.contains("/file1.txt"));
    ok(m.contains("/filez.txt"));

    ok(!m.contains("/"));

    Matcher m2;
    m2.addGlob("file*", "/foo/");
    m2.finalize();
    ok(m2.contains("/foo/file"));
    ok(m2.contains("/foo/file1"));
    ok(m2.contains("/foo/filez"));
    ok(m2.contains("/foo/file.txt"));
    ok(m2.contains("/foo/file1.txt"));
    ok(m2.contains("/foo/filez.txt"));
    ok(!m2.contains("/file"));
    ok(!m2.contains("/file1"));
    ok(!m2.contains("/filez"));
    ok(!m2.contains("/file.txt"));
    ok(!m2.contains("/file1.txt"));
    ok(!m2.contains("/filez.txt"));

    // Matcher::addGlob can accept paths without a trailing slash
    Matcher m3;
    m3.addGlob("file*", "/foo");
    m3.finalize();
    ok(m3.contains("/foo/file"));
    ok(!m3.contains("/file"));
}

// }}}1
/// @}

// === editorconfig-core-test/glob ======================================= {{{1
// For these tests, use absolute paths under `/`.  In the actual
// editorconfig-core-test, the absolute path of the relevant tests directory
// is used.

// TODO throughout the functions below, add the ok(!contains) checks for all
// the matchers that should not match.

/// Tests based on star.in
void
test_core_star()
{

    // --- star.in ---
    // ; test *
    //
    // root=true
    //
    // [a*e.c]
    // key=value
    //
    // [Bar/*]
    // keyb=valueb
    //
    // [*]
    // keyc=valuec
    //

    Matcher ma, mb, mc;  // named after glob/star.in key=value pairs
    ma.addGlob("a*e.c", "/");
    mb.addGlob("Bar/*", "/");
    mc.addGlob("*", "/");
    ma.finalize();
    mb.finalize();
    mc.finalize();


    // matches a single characters
    // new_ec_test_multiline(star_single_ML star.in ace.c "key=value[ \t\n\r]+keyc=valuec[ \t\n\r]*")
    ok(ma.contains("/ace.c"));
    ok(!mb.contains("/ace.c"));
    ok(mc.contains("/ace.c"));


    // matches zero characters
    //new_ec_test_multiline(star_zero_ML star.in ae.c "key=value[ \t\n\r]+keyc=valuec[ \t\n\r]*")
    ok(ma.contains("/ae.c"));
    ok(!mb.contains("/ae.c"));
    ok(mc.contains("/ae.c"));

    // matches multiple characters
    // new_ec_test_multiline(star_multiple_ML star.in abcde.c "key=value[ \t\n\r]+keyc=valuec[ \t\n\r]*")
    ok(ma.contains("/abcde.c"));
    ok(!mb.contains("/abcde.c"));
    ok(mc.contains("/abcde.c"));

    // does not match path separator
    // new_ec_test(star_over_slash star.in a/e.c "^[ \t\n\r]*keyc=valuec[ \t\n\r]*$")
    ok(!ma.contains("/a/e.c"));
    ok(!mb.contains("/a/e.c"));
    ok(mc.contains("/a/e.c"));

    // star after a slash
    // new_ec_test_multiline(star_after_slash_ML star.in Bar/foo.txt "keyb=valueb[ \t\n\r]+keyc=valuec[ \t\n\r]*")
    ok(!ma.contains("/Bar/foo.txt"));
    ok(mb.contains("/Bar/foo.txt"));
    ok(mc.contains("/Bar/foo.txt"));

    // star matches a dot file after slash
    // new_ec_test_multiline(star_matches_dot_file_after_slash_ML star.in Bar/.editorconfig "keyb=valueb[ \t\n\r]+keyc=valuec[ \t\n\r]*")
    ok(!ma.contains("/Bar/.editorconfig"));
    ok(mb.contains("/Bar/.editorconfig"));
    ok(mc.contains("/Bar/.editorconfig"));

    // star matches a dot file
    // new_ec_test(star_matches_dot_file star.in .editorconfig "^keyc=valuec[ \t\n\r]*$")
    ok(!ma.contains("/.editorconfig"));
    ok(!mb.contains("/.editorconfig"));
    ok(mc.contains("/.editorconfig"));
}

/// tests from question.in
void
test_core_question()
{

    // // --- question.in ---
    // ; test ?
    //
    // root=true
    //
    // [som?.c]
    // key=value
    //

    Matcher m({"som?.c"}, "/");

    // # Tests for ?

    // # matches a single character
    // new_ec_test(question_single question.in some.c "^key=value[ \t\n\r]*$")
    ok(m.contains("/some.c"));

    // # does not match zero characters
    // new_ec_test(question_zero question.in som.c "^[ \t\n\r]*$")
    ok(!m.contains("/som.c"));

    // # does not match multiple characters
    // new_ec_test(question_multiple question.in something.c "^[ \t\n\r]*$")
    ok(!m.contains("/something.c"));

    // # does not match slash
    // new_ec_test(question_slash question.in som/.c "^[ \t\n\r]*$")
    ok(!m.contains("/som/.c"));
}

/// Tests for brackets.in
void
test_core_brackets()
{

    // // --- brackets.in ---
    // ; test [ and ]
    //
    // root=true

    // ; Character choice
    // [[ab].a]
    // choice=true
    Matcher choiceTrue({"[ab].a"}, "/");

    // ; Negative character choice
    // [[!ab].b]
    // choice=false
    Matcher choiceFalse({"[!ab].b"}, "/");

    // ; Character range
    // [[d-g].c]
    // range=true
    Matcher rangeTrue({"[d-g].c"}, "/");

    // ; Negative character range
    // [[!d-g].d]
    // range=false
    Matcher rangeFalse({"[!d-g].d"}, "/");

    // ; Range and choice
    // [[abd-g].e]
    // range_and_choice=true
    Matcher rangeAndChoiceTrue({"[abd-g].e"}, "/");

    // ; Choice with dash
    // [[-ab].f]
    // choice_with_dash=true
    Matcher choiceWithDashTrue({"[-ab].f"}, "/");

    // ; Close bracket inside
    // [[\]ab].g]
    // close_inside=true
    Matcher closeInsideTrue({"[\\]ab].g"}, "/");

    // ; Close bracket outside
    // [[ab]].g]
    // close_outside=true
    Matcher closeOutsideTrue({"[ab]].g"}, "/");

    // ; Negative close bracket inside
    // [[!\]ab].g]
    // close_inside=false
    Matcher closeInsideFalse({"[!\\]ab].g"}, "/");

    // ; Negative¬close bracket outside
    // [[!ab]].g]
    // close_outside=false
    Matcher closeOutsideFalse({"[!ab]].g"}, "/");

    // ; Slash inside brackets
    // [ab[e/]cd.i]
    // slash_inside=true
    Matcher slashInsideTrue({"ab[e/]cd.i"}, "/");

    // ; Slash after an half-open bracket
    // [ab[/c]
    // slash_half_open=true
    Matcher slashHalfOpenTrue({"ab[/c"}, "/");

    // # Tests for [ and ]
    //
    // # close bracket inside
    // new_ec_test(brackets_close_inside brackets.in ].g "^close_inside=true[ \t\n\r]*$")
    ok(closeInsideTrue.contains("/].g"));

    // # close bracket outside
    // new_ec_test(brackets_close_outside brackets.in b].g "^close_outside=true[ \t\n\r]*$")
    ok(closeOutsideTrue.contains("/b].g"));

    // # negative close bracket inside
    // new_ec_test(brackets_nclose_inside brackets.in c.g "^close_inside=false[ \t\n\r]*$")
    ok(closeInsideFalse.contains("/c.g"));

    // # negative close bracket outside
    // new_ec_test(brackets_nclose_outside brackets.in c].g "^close_outside=false[ \t\n\r]*$")
    ok(closeOutsideFalse.contains("/c].g"));

    // # character choice
    // new_ec_test(brackets_choice brackets.in a.a "^choice=true[ \t\n\r]*$")
    ok(choiceTrue.contains("/a.a"));

    // # character choice 2
    // new_ec_test(brackets_choice2 brackets.in c.a "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/c.a"));
    ok(!choiceFalse.contains("/c.a"));
    ok(!rangeTrue.contains("/c.a"));
    ok(!rangeFalse.contains("/c.a"));
    ok(!rangeAndChoiceTrue.contains("/c.a"));
    ok(!choiceWithDashTrue.contains("/c.a"));
    ok(!closeInsideTrue.contains("/c.a"));
    ok(!closeOutsideTrue.contains("/c.a"));
    ok(!closeInsideFalse.contains("/c.a"));
    ok(!closeOutsideFalse.contains("/c.a"));
    ok(!slashInsideTrue.contains("/c.a"));
    ok(!slashHalfOpenTrue.contains("/c.a"));

    // # negative character choice
    // new_ec_test(brackets_nchoice brackets.in c.b "^choice=false[ \t\n\r]*$")
    ok(choiceFalse.contains("/c.b"));

    // # negative character choice 2
    // new_ec_test(brackets_nchoice2 brackets.in a.b "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/a.b"));
    ok(!choiceFalse.contains("/a.b"));
    ok(!rangeTrue.contains("/a.b"));
    ok(!rangeFalse.contains("/a.b"));
    ok(!rangeAndChoiceTrue.contains("/a.b"));
    ok(!choiceWithDashTrue.contains("/a.b"));
    ok(!closeInsideTrue.contains("/a.b"));
    ok(!closeOutsideTrue.contains("/a.b"));
    ok(!closeInsideFalse.contains("/a.b"));
    ok(!closeOutsideFalse.contains("/a.b"));
    ok(!slashInsideTrue.contains("/a.b"));
    ok(!slashHalfOpenTrue.contains("/a.b"));

    // # character range
    // new_ec_test(brackets_range brackets.in f.c "^range=true[ \t\n\r]*$")
    ok(rangeTrue.contains("/f.c"));

    // # character range 2
    // new_ec_test(brackets_range2 brackets.in h.c "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/h.c"));
    ok(!choiceFalse.contains("/h.c"));
    ok(!rangeTrue.contains("/h.c"));
    ok(!rangeFalse.contains("/h.c"));
    ok(!rangeAndChoiceTrue.contains("/h.c"));
    ok(!choiceWithDashTrue.contains("/h.c"));
    ok(!closeInsideTrue.contains("/h.c"));
    ok(!closeOutsideTrue.contains("/h.c"));
    ok(!closeInsideFalse.contains("/h.c"));
    ok(!closeOutsideFalse.contains("/h.c"));
    ok(!slashInsideTrue.contains("/h.c"));
    ok(!slashHalfOpenTrue.contains("/h.c"));

    // # negative character range
    // new_ec_test(brackets_nrange brackets.in h.d "^range=false[ \t\n\r]*$")
    ok(rangeFalse.contains("/h.d"));

    // # negative character range 2
    // new_ec_test(brackets_nrange2 brackets.in f.d "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/f.d"));
    ok(!choiceFalse.contains("/f.d"));
    ok(!rangeTrue.contains("/f.d"));
    ok(!rangeFalse.contains("/f.d"));
    ok(!rangeAndChoiceTrue.contains("/f.d"));
    ok(!choiceWithDashTrue.contains("/f.d"));
    ok(!closeInsideTrue.contains("/f.d"));
    ok(!closeOutsideTrue.contains("/f.d"));
    ok(!closeInsideFalse.contains("/f.d"));
    ok(!closeOutsideFalse.contains("/f.d"));
    ok(!slashInsideTrue.contains("/f.d"));
    ok(!slashHalfOpenTrue.contains("/f.d"));

    // # range and choice
    // new_ec_test(brackets_range_and_choice brackets.in e.e
    //     "^range_and_choice=true[ \t\n\r]*$")
    ok(rangeAndChoiceTrue.contains("/e.e"));

    // # character choice with a dash
    // new_ec_test(brackets_choice_with_dash brackets.in -.f
    //     "^choice_with_dash=true[ \t\n\r]*$")
    ok(choiceWithDashTrue.contains("/-.f"));

    // # slash inside brackets
    // new_ec_test(brackets_slash_inside1 brackets.in ab/cd.i
    //     "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/ab/cd.i"));
    ok(!choiceFalse.contains("/ab/cd.i"));
    ok(!rangeTrue.contains("/ab/cd.i"));
    ok(!rangeFalse.contains("/ab/cd.i"));
    ok(!rangeAndChoiceTrue.contains("/ab/cd.i"));
    ok(!choiceWithDashTrue.contains("/ab/cd.i"));
    ok(!closeInsideTrue.contains("/ab/cd.i"));
    ok(!closeOutsideTrue.contains("/ab/cd.i"));
    ok(!closeInsideFalse.contains("/ab/cd.i"));
    ok(!closeOutsideFalse.contains("/ab/cd.i"));
    ok(!slashInsideTrue.contains("/ab/cd.i"));
    ok(!slashHalfOpenTrue.contains("/ab/cd.i"));

    // new_ec_test(brackets_slash_inside2 brackets.in abecd.i
    //     "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/abecd.i"));
    ok(!choiceFalse.contains("/abecd.i"));
    ok(!rangeTrue.contains("/abecd.i"));
    ok(!rangeFalse.contains("/abecd.i"));
    ok(!rangeAndChoiceTrue.contains("/abecd.i"));
    ok(!choiceWithDashTrue.contains("/abecd.i"));
    ok(!closeInsideTrue.contains("/abecd.i"));
    ok(!closeOutsideTrue.contains("/abecd.i"));
    ok(!closeInsideFalse.contains("/abecd.i"));
    ok(!closeOutsideFalse.contains("/abecd.i"));
    ok(!slashInsideTrue.contains("/abecd.i"));
    ok(!slashHalfOpenTrue.contains("/abecd.i"));

    // new_ec_test(brackets_slash_inside3 brackets.in ab[e/]cd.i
    //     "^slash_inside=true[ \t\n\r]*$")
    ok(slashInsideTrue.contains("/ab[e/]cd.i"));

    // new_ec_test(brackets_slash_inside4 brackets.in ab[/c
    //     "^slash_half_open=true[ \t\n\r]*$")
    ok(slashHalfOpenTrue.contains("/ab[/c"));
}

/// Tests for braces.in (`{` and `}`)
void
test_core_braces()
{

    // // --- braces.in ---
    // ; test { and }
    //
    // root=true
    //
    // ; word choice
    // [*.{py,js,html}]
    // choice=true
    Matcher choiceTrue({"*.{py,js,html}"}, "/");

    // ; single choice
    // [{single}.b]
    // choice=single
    Matcher choiceSingle({"{single}.b"}, "/");

    // ; empty choice
    // [{}.c]
    // empty=all
    Matcher emptyAll({"{}.c"}, "/");

    // ; choice with empty word
    // [a{b,c,}.d]
    // empty=word
    Matcher emptyWord({"a{b,c,}.d"}, "/");

    // ; choice with empty words
    // [a{,b,,c,}.e]
    // empty=words
    Matcher emptyWords({"a{,b,,c,}.e"}, "/");

    // ; no closing brace
    // [{.f]
    // closing=false
    Matcher closingFalse({"{.f"}, "/");

    // ; nested braces
    // [{word,{also},this}.g]
    // nested=true
    Matcher nestedTrue({"{word,{also},this}.g"}, "/");

    // ; closing inside beginning
    // [{},b}.h]
    // closing=inside
    Matcher closingInside({"{},b}.h"}, "/");

    // ; opening inside beginning
    // [{{,b,c{d}.i]
    // unmatched=true
    Matcher unmatchedTrue({"{{,b,c{d}.i"}, "/");

    // ; escaped comma
    // [{a\,b,cd}.txt]
    // comma=yes
    Matcher commaYes({R"({a\,b,cd}.txt)"}, "/");

    // ; escaped closing brace
    // [{e,\},f}.txt]
    // closing=yes
    Matcher closingYes({R"({e,\},f}.txt)"}, "/");

    // ; escaped backslash
    // [{g,\\,i}.txt]
    // backslash=yes
    Matcher backslashYes({R"({g,\\,i}.txt)"}, "/");

    // ; patterns nested in braces
    // [{some,a{*c,b}[ef]}.j]
    // patterns=nested
    Matcher patternsNested({"{some,a{*c,b}[ef]}.j"}, "/");

    // ; numeric braces
    // [{3..120}]
    // number=true
    Matcher numberTrue({"{3..120}"}, "/");

    // ; alphabetical
    // [{aardvark..antelope}]
    // words=a
    Matcher wordsA({"{aardvark..antelope}"}, "/");

    // # word choice
    // new_ec_test(braces_word_choice1 braces.in test.py "^choice=true[ \t\n\r]*$")
    ok(choiceTrue.contains("/test.py"));

    // new_ec_test(braces_word_choice2 braces.in test.js "^choice=true[ \t\n\r]*$")
    ok(choiceTrue.contains("/test.js"));

    // new_ec_test(braces_word_choice3 braces.in test.html "^choice=true[ \t\n\r]*$")
    ok(choiceTrue.contains("/test.html"));

    // new_ec_test(braces_word_choice4 braces.in test.pyc "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/test.pyc"));
    ok(!choiceSingle.contains("/test.pyc"));
    ok(!emptyAll.contains("/test.pyc"));
    ok(!emptyWord.contains("/test.pyc"));
    ok(!emptyWords.contains("/test.pyc"));
    ok(!closingFalse.contains("/test.pyc"));
    ok(!nestedTrue.contains("/test.pyc"));
    ok(!closingInside.contains("/test.pyc"));
    ok(!unmatchedTrue.contains("/test.pyc"));
    ok(!commaYes.contains("/test.pyc"));
    ok(!closingYes.contains("/test.pyc"));
    ok(!backslashYes.contains("/test.pyc"));
    ok(!patternsNested.contains("/test.pyc"));
    ok(!numberTrue.contains("/test.pyc"));
    ok(!wordsA.contains("/test.pyc"));

    // # single choice
    // new_ec_test(braces_single_choice braces.in {single}.b "^choice=single[ \t\n\r]*$")
    ok(choiceSingle.contains("/{single}.b"));

    // new_ec_test(braces_single_choice_negative braces.in .b "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/.b"));
    ok(!choiceSingle.contains("/.b"));
    ok(!emptyAll.contains("/.b"));
    ok(!emptyWord.contains("/.b"));
    ok(!emptyWords.contains("/.b"));
    ok(!closingFalse.contains("/.b"));
    ok(!nestedTrue.contains("/.b"));
    ok(!closingInside.contains("/.b"));
    ok(!unmatchedTrue.contains("/.b"));
    ok(!commaYes.contains("/.b"));
    ok(!closingYes.contains("/.b"));
    ok(!backslashYes.contains("/.b"));
    ok(!patternsNested.contains("/.b"));
    ok(!numberTrue.contains("/.b"));
    ok(!wordsA.contains("/.b"));

    // # empty choice
    // new_ec_test(braces_empty_choice braces.in {}.c "^empty=all[ \t\n\r]*$")
    ok(emptyAll.contains("/{}.c"));

    // new_ec_test(braces_empty_choice_negative braces.in .c "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/.c"));
    ok(!choiceSingle.contains("/.c"));
    ok(!emptyAll.contains("/.c"));
    ok(!emptyWord.contains("/.c"));
    ok(!emptyWords.contains("/.c"));
    ok(!closingFalse.contains("/.c"));
    ok(!nestedTrue.contains("/.c"));
    ok(!closingInside.contains("/.c"));
    ok(!unmatchedTrue.contains("/.c"));
    ok(!commaYes.contains("/.c"));
    ok(!closingYes.contains("/.c"));
    ok(!backslashYes.contains("/.c"));
    ok(!patternsNested.contains("/.c"));
    ok(!numberTrue.contains("/.c"));
    ok(!wordsA.contains("/.c"));

    // # choice with empty word
    // new_ec_test(braces_empty_word1 braces.in a.d "^empty=word[ \t\n\r]*$")
    ok(emptyWord.contains("/a.d"));

    // new_ec_test(braces_empty_word2 braces.in ab.d "^empty=word[ \t\n\r]*$")
    ok(emptyWord.contains("/ab.d"));

    // new_ec_test(braces_empty_word3 braces.in ac.d "^empty=word[ \t\n\r]*$")
    ok(emptyWord.contains("/ac.d"));

    // new_ec_test(braces_empty_word4 braces.in a,.d "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/a,.d"));
    ok(!choiceSingle.contains("/a,.d"));
    ok(!emptyAll.contains("/a,.d"));
    ok(!emptyWord.contains("/a,.d"));
    ok(!emptyWords.contains("/a,.d"));
    ok(!closingFalse.contains("/a,.d"));
    ok(!nestedTrue.contains("/a,.d"));
    ok(!closingInside.contains("/a,.d"));
    ok(!unmatchedTrue.contains("/a,.d"));
    ok(!commaYes.contains("/a,.d"));
    ok(!closingYes.contains("/a,.d"));
    ok(!backslashYes.contains("/a,.d"));
    ok(!patternsNested.contains("/a,.d"));
    ok(!numberTrue.contains("/a,.d"));
    ok(!wordsA.contains("/a,.d"));

    // # choice with empty words
    // new_ec_test(braces_empty_words1 braces.in a.e "^empty=words[ \t\n\r]*$")
    ok(emptyWords.contains("/a.e"));

    // new_ec_test(braces_empty_words2 braces.in ab.e "^empty=words[ \t\n\r]*$")
    ok(emptyWords.contains("/ab.e"));

    // new_ec_test(braces_empty_words3 braces.in ac.e "^empty=words[ \t\n\r]*$")
    ok(emptyWords.contains("/ac.e"));

    // new_ec_test(braces_empty_words4 braces.in a,.e "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/a,.e"));
    ok(!choiceSingle.contains("/a,.e"));
    ok(!emptyAll.contains("/a,.e"));
    ok(!emptyWord.contains("/a,.e"));
    ok(!emptyWords.contains("/a,.e"));
    ok(!closingFalse.contains("/a,.e"));
    ok(!nestedTrue.contains("/a,.e"));
    ok(!closingInside.contains("/a,.e"));
    ok(!unmatchedTrue.contains("/a,.e"));
    ok(!commaYes.contains("/a,.e"));
    ok(!closingYes.contains("/a,.e"));
    ok(!backslashYes.contains("/a,.e"));
    ok(!patternsNested.contains("/a,.e"));
    ok(!numberTrue.contains("/a,.e"));
    ok(!wordsA.contains("/a,.e"));

    // # no closing brace
    // new_ec_test(braces_no_closing braces.in {.f "^closing=false[ \t\n\r]*$")
    ok(closingFalse.contains("/{.f"));

    // new_ec_test(braces_no_closing_negative braces.in .f "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/.f"));
    ok(!choiceSingle.contains("/.f"));
    ok(!emptyAll.contains("/.f"));
    ok(!emptyWord.contains("/.f"));
    ok(!emptyWords.contains("/.f"));
    ok(!closingFalse.contains("/.f"));
    ok(!nestedTrue.contains("/.f"));
    ok(!closingInside.contains("/.f"));
    ok(!unmatchedTrue.contains("/.f"));
    ok(!commaYes.contains("/.f"));
    ok(!closingYes.contains("/.f"));
    ok(!backslashYes.contains("/.f"));
    ok(!patternsNested.contains("/.f"));
    ok(!numberTrue.contains("/.f"));
    ok(!wordsA.contains("/.f"));

    // # nested braces
    // new_ec_test(braces_nested1 braces.in word,this}.g "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/word,this}.g"));
    ok(!choiceSingle.contains("/word,this}.g"));
    ok(!emptyAll.contains("/word,this}.g"));
    ok(!emptyWord.contains("/word,this}.g"));
    ok(!emptyWords.contains("/word,this}.g"));
    ok(!closingFalse.contains("/word,this}.g"));
    ok(!nestedTrue.contains("/word,this}.g"));
    ok(!closingInside.contains("/word,this}.g"));
    ok(!unmatchedTrue.contains("/word,this}.g"));
    ok(!commaYes.contains("/word,this}.g"));
    ok(!closingYes.contains("/word,this}.g"));
    ok(!backslashYes.contains("/word,this}.g"));
    ok(!patternsNested.contains("/word,this}.g"));
    ok(!numberTrue.contains("/word,this}.g"));
    ok(!wordsA.contains("/word,this}.g"));

    // new_ec_test(braces_nested2 braces.in {also,this}.g "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/{also,this}.g"));
    ok(!choiceSingle.contains("/{also,this}.g"));
    ok(!emptyAll.contains("/{also,this}.g"));
    ok(!emptyWord.contains("/{also,this}.g"));
    ok(!emptyWords.contains("/{also,this}.g"));
    ok(!closingFalse.contains("/{also,this}.g"));
    ok(!nestedTrue.contains("/{also,this}.g"));
    ok(!closingInside.contains("/{also,this}.g"));
    ok(!unmatchedTrue.contains("/{also,this}.g"));
    ok(!commaYes.contains("/{also,this}.g"));
    ok(!closingYes.contains("/{also,this}.g"));
    ok(!backslashYes.contains("/{also,this}.g"));
    ok(!patternsNested.contains("/{also,this}.g"));
    ok(!numberTrue.contains("/{also,this}.g"));
    ok(!wordsA.contains("/{also,this}.g"));

    // new_ec_test(braces_nested3 braces.in word.g "^nested=true[ \t\n\r]*$")
    ok(nestedTrue.contains("/word.g"));

    // new_ec_test(braces_nested4 braces.in {also}.g "^nested=true[ \t\n\r]*$")
    ok(nestedTrue.contains("/{also}.g"));

    // new_ec_test(braces_nested5 braces.in this.g "^nested=true[ \t\n\r]*$")
    ok(nestedTrue.contains("/this.g"));

    // # closing inside beginning
    // new_ec_test(braces_closing_in_beginning braces.in {},b}.h "^closing=inside[ \t\n\r]*$")
    ok(closingInside.contains("/{},b}.h"));

    // # missing closing braces
    // new_ec_test(braces_unmatched1 braces.in {{,b,c{d}.i "^unmatched=true[ \t\n\r]*$")
    ok(unmatchedTrue.contains("/{{,b,c{d}.i"));

    // new_ec_test(braces_unmatched2 braces.in {.i "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/{.i"));
    ok(!choiceSingle.contains("/{.i"));
    ok(!emptyAll.contains("/{.i"));
    ok(!emptyWord.contains("/{.i"));
    ok(!emptyWords.contains("/{.i"));
    ok(!closingFalse.contains("/{.i"));
    ok(!nestedTrue.contains("/{.i"));
    ok(!closingInside.contains("/{.i"));
    ok(!unmatchedTrue.contains("/{.i"));
    ok(!commaYes.contains("/{.i"));
    ok(!closingYes.contains("/{.i"));
    ok(!backslashYes.contains("/{.i"));
    ok(!patternsNested.contains("/{.i"));
    ok(!numberTrue.contains("/{.i"));
    ok(!wordsA.contains("/{.i"));

    // new_ec_test(braces_unmatched3 braces.in b.i "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/b.i"));
    ok(!choiceSingle.contains("/b.i"));
    ok(!emptyAll.contains("/b.i"));
    ok(!emptyWord.contains("/b.i"));
    ok(!emptyWords.contains("/b.i"));
    ok(!closingFalse.contains("/b.i"));
    ok(!nestedTrue.contains("/b.i"));
    ok(!closingInside.contains("/b.i"));
    ok(!unmatchedTrue.contains("/b.i"));
    ok(!commaYes.contains("/b.i"));
    ok(!closingYes.contains("/b.i"));
    ok(!backslashYes.contains("/b.i"));
    ok(!patternsNested.contains("/b.i"));
    ok(!numberTrue.contains("/b.i"));
    ok(!wordsA.contains("/b.i"));

    // new_ec_test(braces_unmatched4 braces.in c{d.i "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/c{d.i"));
    ok(!choiceSingle.contains("/c{d.i"));
    ok(!emptyAll.contains("/c{d.i"));
    ok(!emptyWord.contains("/c{d.i"));
    ok(!emptyWords.contains("/c{d.i"));
    ok(!closingFalse.contains("/c{d.i"));
    ok(!nestedTrue.contains("/c{d.i"));
    ok(!closingInside.contains("/c{d.i"));
    ok(!unmatchedTrue.contains("/c{d.i"));
    ok(!commaYes.contains("/c{d.i"));
    ok(!closingYes.contains("/c{d.i"));
    ok(!backslashYes.contains("/c{d.i"));
    ok(!patternsNested.contains("/c{d.i"));
    ok(!numberTrue.contains("/c{d.i"));
    ok(!wordsA.contains("/c{d.i"));

    // new_ec_test(braces_unmatched5 braces.in .i "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/.i"));
    ok(!choiceSingle.contains("/.i"));
    ok(!emptyAll.contains("/.i"));
    ok(!emptyWord.contains("/.i"));
    ok(!emptyWords.contains("/.i"));
    ok(!closingFalse.contains("/.i"));
    ok(!nestedTrue.contains("/.i"));
    ok(!closingInside.contains("/.i"));
    ok(!unmatchedTrue.contains("/.i"));
    ok(!commaYes.contains("/.i"));
    ok(!closingYes.contains("/.i"));
    ok(!backslashYes.contains("/.i"));
    ok(!patternsNested.contains("/.i"));
    ok(!numberTrue.contains("/.i"));
    ok(!wordsA.contains("/.i"));

    // # escaped comma
    // new_ec_test(braces_escaped_comma1 braces.in a,b.txt "^comma=yes[ \t\n\r]*$")
    ok(commaYes.contains("/a,b.txt"));

    // new_ec_test(braces_escaped_comma2 braces.in a.txt "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/a.txt"));
    ok(!choiceSingle.contains("/a.txt"));
    ok(!emptyAll.contains("/a.txt"));
    ok(!emptyWord.contains("/a.txt"));
    ok(!emptyWords.contains("/a.txt"));
    ok(!closingFalse.contains("/a.txt"));
    ok(!nestedTrue.contains("/a.txt"));
    ok(!closingInside.contains("/a.txt"));
    ok(!unmatchedTrue.contains("/a.txt"));
    ok(!commaYes.contains("/a.txt"));
    ok(!closingYes.contains("/a.txt"));
    ok(!backslashYes.contains("/a.txt"));
    ok(!patternsNested.contains("/a.txt"));
    ok(!numberTrue.contains("/a.txt"));
    ok(!wordsA.contains("/a.txt"));

    // new_ec_test(braces_escaped_comma3 braces.in cd.txt "^comma=yes[ \t\n\r]*$")
    ok(commaYes.contains("/cd.txt"));

    // # escaped closing brace
    // new_ec_test(braces_escaped_brace1 braces.in e.txt "^closing=yes[ \t\n\r]*$")
    ok(closingYes.contains("/e.txt"));

    // new_ec_test(braces_escaped_brace2 braces.in }.txt "^closing=yes[ \t\n\r]*$")
    ok(closingYes.contains("/}.txt"));

    // new_ec_test(braces_escaped_brace3 braces.in f.txt "^closing=yes[ \t\n\r]*$")
    ok(closingYes.contains("/f.txt"));

    // TODO handle this test
    // # escaped backslash
    // new_ec_test(braces_escaped_backslash1 braces.in g.txt "^backslash=yes[ \t\n\r]*$")
    // if((NOT WIN32) AND (NOT CYGWIN))    # this case is impossible on Windows.
    //     new_ec_test(braces_escaped_backslash2 braces.in "\\.txt" "^backslash=yes[ \t\n\r]*$")
    // endif()

    // new_ec_test(braces_escaped_backslash3 braces.in i.txt "^backslash=yes[ \t\n\r]*$")
    ok(backslashYes.contains("/i.txt"));

    // # patterns nested in braces
    // new_ec_test(braces_patterns_nested1 braces.in some.j "^patterns=nested[ \t\n\r]*$")
    ok(patternsNested.contains("/some.j"));

    // new_ec_test(braces_patterns_nested2 braces.in abe.j "^patterns=nested[ \t\n\r]*$")
    ok(patternsNested.contains("/abe.j"));

    // new_ec_test(braces_patterns_nested3 braces.in abf.j "^patterns=nested[ \t\n\r]*$")
    ok(patternsNested.contains("/abf.j"));

    // new_ec_test(braces_patterns_nested4 braces.in abg.j "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/abg.j"));
    ok(!choiceSingle.contains("/abg.j"));
    ok(!emptyAll.contains("/abg.j"));
    ok(!emptyWord.contains("/abg.j"));
    ok(!emptyWords.contains("/abg.j"));
    ok(!closingFalse.contains("/abg.j"));
    ok(!nestedTrue.contains("/abg.j"));
    ok(!closingInside.contains("/abg.j"));
    ok(!unmatchedTrue.contains("/abg.j"));
    ok(!commaYes.contains("/abg.j"));
    ok(!closingYes.contains("/abg.j"));
    ok(!backslashYes.contains("/abg.j"));
    ok(!patternsNested.contains("/abg.j"));
    ok(!numberTrue.contains("/abg.j"));
    ok(!wordsA.contains("/abg.j"));

    // new_ec_test(braces_patterns_nested5 braces.in ace.j "^patterns=nested[ \t\n\r]*$")
    ok(patternsNested.contains("/ace.j"));

    // new_ec_test(braces_patterns_nested6 braces.in acf.j "^patterns=nested[ \t\n\r]*$")
    ok(patternsNested.contains("/acf.j"));

    // new_ec_test(braces_patterns_nested7 braces.in acg.j "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/acg.j"));
    ok(!choiceSingle.contains("/acg.j"));
    ok(!emptyAll.contains("/acg.j"));
    ok(!emptyWord.contains("/acg.j"));
    ok(!emptyWords.contains("/acg.j"));
    ok(!closingFalse.contains("/acg.j"));
    ok(!nestedTrue.contains("/acg.j"));
    ok(!closingInside.contains("/acg.j"));
    ok(!unmatchedTrue.contains("/acg.j"));
    ok(!commaYes.contains("/acg.j"));
    ok(!closingYes.contains("/acg.j"));
    ok(!backslashYes.contains("/acg.j"));
    ok(!patternsNested.contains("/acg.j"));
    ok(!numberTrue.contains("/acg.j"));
    ok(!wordsA.contains("/acg.j"));

    // new_ec_test(braces_patterns_nested8 braces.in abce.j "^patterns=nested[ \t\n\r]*$")
    ok(patternsNested.contains("/abce.j"));

    // new_ec_test(braces_patterns_nested9 braces.in abcf.j "^patterns=nested[ \t\n\r]*$")
    ok(patternsNested.contains("/abcf.j"));

    // new_ec_test(braces_patterns_nested10 braces.in abcg.j "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/abcg.j"));
    ok(!choiceSingle.contains("/abcg.j"));
    ok(!emptyAll.contains("/abcg.j"));
    ok(!emptyWord.contains("/abcg.j"));
    ok(!emptyWords.contains("/abcg.j"));
    ok(!closingFalse.contains("/abcg.j"));
    ok(!nestedTrue.contains("/abcg.j"));
    ok(!closingInside.contains("/abcg.j"));
    ok(!unmatchedTrue.contains("/abcg.j"));
    ok(!commaYes.contains("/abcg.j"));
    ok(!closingYes.contains("/abcg.j"));
    ok(!backslashYes.contains("/abcg.j"));
    ok(!patternsNested.contains("/abcg.j"));
    ok(!numberTrue.contains("/abcg.j"));
    ok(!wordsA.contains("/abcg.j"));

    // new_ec_test(braces_patterns_nested11 braces.in ae.j "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/ae.j"));
    ok(!choiceSingle.contains("/ae.j"));
    ok(!emptyAll.contains("/ae.j"));
    ok(!emptyWord.contains("/ae.j"));
    ok(!emptyWords.contains("/ae.j"));
    ok(!closingFalse.contains("/ae.j"));
    ok(!nestedTrue.contains("/ae.j"));
    ok(!closingInside.contains("/ae.j"));
    ok(!unmatchedTrue.contains("/ae.j"));
    ok(!commaYes.contains("/ae.j"));
    ok(!closingYes.contains("/ae.j"));
    ok(!backslashYes.contains("/ae.j"));
    ok(!patternsNested.contains("/ae.j"));
    ok(!numberTrue.contains("/ae.j"));
    ok(!wordsA.contains("/ae.j"));

    // new_ec_test(braces_patterns_nested12 braces.in .j "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/.j"));
    ok(!choiceSingle.contains("/.j"));
    ok(!emptyAll.contains("/.j"));
    ok(!emptyWord.contains("/.j"));
    ok(!emptyWords.contains("/.j"));
    ok(!closingFalse.contains("/.j"));
    ok(!nestedTrue.contains("/.j"));
    ok(!closingInside.contains("/.j"));
    ok(!unmatchedTrue.contains("/.j"));
    ok(!commaYes.contains("/.j"));
    ok(!closingYes.contains("/.j"));
    ok(!backslashYes.contains("/.j"));
    ok(!patternsNested.contains("/.j"));
    ok(!numberTrue.contains("/.j"));
    ok(!wordsA.contains("/.j"));

    // # numeric brace range
    // new_ec_test(braces_numeric_range1 braces.in 1 "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/1"));
    ok(!choiceSingle.contains("/1"));
    ok(!emptyAll.contains("/1"));
    ok(!emptyWord.contains("/1"));
    ok(!emptyWords.contains("/1"));
    ok(!closingFalse.contains("/1"));
    ok(!nestedTrue.contains("/1"));
    ok(!closingInside.contains("/1"));
    ok(!unmatchedTrue.contains("/1"));
    ok(!commaYes.contains("/1"));
    ok(!closingYes.contains("/1"));
    ok(!backslashYes.contains("/1"));
    ok(!patternsNested.contains("/1"));
    ok(!numberTrue.contains("/1"));
    ok(!wordsA.contains("/1"));

    // new_ec_test(braces_numeric_range2 braces.in 3 "^number=true[ \t\n\r]*$")
    ok(numberTrue.contains("/3"));

    // new_ec_test(braces_numeric_range3 braces.in 15 "^number=true[ \t\n\r]*$")
    ok(numberTrue.contains("/15"));

    // new_ec_test(braces_numeric_range4 braces.in 60 "^number=true[ \t\n\r]*$")
    ok(numberTrue.contains("/60"));

    // new_ec_test(braces_numeric_range5 braces.in 5a "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/5a"));
    ok(!choiceSingle.contains("/5a"));
    ok(!emptyAll.contains("/5a"));
    ok(!emptyWord.contains("/5a"));
    ok(!emptyWords.contains("/5a"));
    ok(!closingFalse.contains("/5a"));
    ok(!nestedTrue.contains("/5a"));
    ok(!closingInside.contains("/5a"));
    ok(!unmatchedTrue.contains("/5a"));
    ok(!commaYes.contains("/5a"));
    ok(!closingYes.contains("/5a"));
    ok(!backslashYes.contains("/5a"));
    ok(!patternsNested.contains("/5a"));
    ok(!numberTrue.contains("/5a"));
    ok(!wordsA.contains("/5a"));

    // new_ec_test(braces_numeric_range6 braces.in 120 "^number=true[ \t\n\r]*$")
    ok(numberTrue.contains("/120"));

    // new_ec_test(braces_numeric_range7 braces.in 121 "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/121"));
    ok(!choiceSingle.contains("/121"));
    ok(!emptyAll.contains("/121"));
    ok(!emptyWord.contains("/121"));
    ok(!emptyWords.contains("/121"));
    ok(!closingFalse.contains("/121"));
    ok(!nestedTrue.contains("/121"));
    ok(!closingInside.contains("/121"));
    ok(!unmatchedTrue.contains("/121"));
    ok(!commaYes.contains("/121"));
    ok(!closingYes.contains("/121"));
    ok(!backslashYes.contains("/121"));
    ok(!patternsNested.contains("/121"));
    ok(!numberTrue.contains("/121"));
    ok(!wordsA.contains("/121"));

    // new_ec_test(braces_numeric_range8 braces.in 060 "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/060"));
    ok(!choiceSingle.contains("/060"));
    ok(!emptyAll.contains("/060"));
    ok(!emptyWord.contains("/060"));
    ok(!emptyWords.contains("/060"));
    ok(!closingFalse.contains("/060"));
    ok(!nestedTrue.contains("/060"));
    ok(!closingInside.contains("/060"));
    ok(!unmatchedTrue.contains("/060"));
    ok(!commaYes.contains("/060"));
    ok(!closingYes.contains("/060"));
    ok(!backslashYes.contains("/060"));
    ok(!patternsNested.contains("/060"));
    ok(!numberTrue.contains("/060"));
    ok(!wordsA.contains("/060"));

    // # alphabetical brace range: letters should not be considered for ranges
    // new_ec_test(braces_alpha_range1 braces.in {aardvark..antelope} "^words=a[ \t\n\r]*$")
    ok(wordsA.contains("/{aardvark..antelope}"));

    // new_ec_test(braces_alpha_range2 braces.in a "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/a"));
    ok(!choiceSingle.contains("/a"));
    ok(!emptyAll.contains("/a"));
    ok(!emptyWord.contains("/a"));
    ok(!emptyWords.contains("/a"));
    ok(!closingFalse.contains("/a"));
    ok(!nestedTrue.contains("/a"));
    ok(!closingInside.contains("/a"));
    ok(!unmatchedTrue.contains("/a"));
    ok(!commaYes.contains("/a"));
    ok(!closingYes.contains("/a"));
    ok(!backslashYes.contains("/a"));
    ok(!patternsNested.contains("/a"));
    ok(!numberTrue.contains("/a"));
    ok(!wordsA.contains("/a"));

    // new_ec_test(braces_alpha_range3 braces.in aardvark "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/aardvark"));
    ok(!choiceSingle.contains("/aardvark"));
    ok(!emptyAll.contains("/aardvark"));
    ok(!emptyWord.contains("/aardvark"));
    ok(!emptyWords.contains("/aardvark"));
    ok(!closingFalse.contains("/aardvark"));
    ok(!nestedTrue.contains("/aardvark"));
    ok(!closingInside.contains("/aardvark"));
    ok(!unmatchedTrue.contains("/aardvark"));
    ok(!commaYes.contains("/aardvark"));
    ok(!closingYes.contains("/aardvark"));
    ok(!backslashYes.contains("/aardvark"));
    ok(!patternsNested.contains("/aardvark"));
    ok(!numberTrue.contains("/aardvark"));
    ok(!wordsA.contains("/aardvark"));

    // new_ec_test(braces_alpha_range4 braces.in agreement "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/agreement"));
    ok(!choiceSingle.contains("/agreement"));
    ok(!emptyAll.contains("/agreement"));
    ok(!emptyWord.contains("/agreement"));
    ok(!emptyWords.contains("/agreement"));
    ok(!closingFalse.contains("/agreement"));
    ok(!nestedTrue.contains("/agreement"));
    ok(!closingInside.contains("/agreement"));
    ok(!unmatchedTrue.contains("/agreement"));
    ok(!commaYes.contains("/agreement"));
    ok(!closingYes.contains("/agreement"));
    ok(!backslashYes.contains("/agreement"));
    ok(!patternsNested.contains("/agreement"));
    ok(!numberTrue.contains("/agreement"));
    ok(!wordsA.contains("/agreement"));

    // new_ec_test(braces_alpha_range5 braces.in antelope "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/antelope"));
    ok(!choiceSingle.contains("/antelope"));
    ok(!emptyAll.contains("/antelope"));
    ok(!emptyWord.contains("/antelope"));
    ok(!emptyWords.contains("/antelope"));
    ok(!closingFalse.contains("/antelope"));
    ok(!nestedTrue.contains("/antelope"));
    ok(!closingInside.contains("/antelope"));
    ok(!unmatchedTrue.contains("/antelope"));
    ok(!commaYes.contains("/antelope"));
    ok(!closingYes.contains("/antelope"));
    ok(!backslashYes.contains("/antelope"));
    ok(!patternsNested.contains("/antelope"));
    ok(!numberTrue.contains("/antelope"));
    ok(!wordsA.contains("/antelope"));

    // new_ec_test(braces_alpha_range6 braces.in antimatter "^[ \t\n\r]*$")
    ok(!choiceTrue.contains("/antimatter"));
    ok(!choiceSingle.contains("/antimatter"));
    ok(!emptyAll.contains("/antimatter"));
    ok(!emptyWord.contains("/antimatter"));
    ok(!emptyWords.contains("/antimatter"));
    ok(!closingFalse.contains("/antimatter"));
    ok(!nestedTrue.contains("/antimatter"));
    ok(!closingInside.contains("/antimatter"));
    ok(!unmatchedTrue.contains("/antimatter"));
    ok(!commaYes.contains("/antimatter"));
    ok(!closingYes.contains("/antimatter"));
    ok(!backslashYes.contains("/antimatter"));
    ok(!patternsNested.contains("/antimatter"));
    ok(!numberTrue.contains("/antimatter"));
    ok(!wordsA.contains("/antimatter"));

} // test_core_braces()

/// Tests for globstar (`**`)
void
test_core_globstar()
{
    // --- star_star.in ---
    // ; test **
    //
    // root=true
    //
    // [a**z.c]
    // key1=value1
    Matcher kv1({"a**z.c"}, "/");

    //
    // [b/**z.c]
    // key2=value2
    Matcher kv2({"b/**z.c"}, "/");

    //
    // [c**/z.c]
    // key3=value3
    Matcher kv3({"c**/z.c"}, "/");

    //
    // [d/**/z.c]
    // key4=value4
    Matcher kv4({"d/**/z.c"}, "/");

    // # matches over path separator
    // new_ec_test(star_star_over_separator1 star_star.in a/z.c "^key1=value1[ \t\n\r]*$")
    ok(kv1.contains("/a/z.c"));

    // new_ec_test(star_star_over_separator2 star_star.in amnz.c "^key1=value1[ \t\n\r]*$")
    ok(kv1.contains("/amnz.c"));

    // new_ec_test(star_star_over_separator3 star_star.in am/nz.c "^key1=value1[ \t\n\r]*$")
    ok(kv1.contains("/am/nz.c"));

    // new_ec_test(star_star_over_separator4 star_star.in a/mnz.c "^key1=value1[ \t\n\r]*$")
    ok(kv1.contains("/a/mnz.c"));

    // new_ec_test(star_star_over_separator5 star_star.in amn/z.c "^key1=value1[ \t\n\r]*$")
    ok(kv1.contains("/amn/z.c"));

    // new_ec_test(star_star_over_separator6 star_star.in a/mn/z.c "^key1=value1[ \t\n\r]*$")
    ok(kv1.contains("/a/mn/z.c"));

    // new_ec_test(star_star_over_separator7 star_star.in b/z.c "^key2=value2[ \t\n\r]*$")
    ok(kv2.contains("/b/z.c"));

    // new_ec_test(star_star_over_separator8 star_star.in b/mnz.c "^key2=value2[ \t\n\r]*$")
    ok(kv2.contains("/b/mnz.c"));

    // new_ec_test(star_star_over_separator9 star_star.in b/mn/z.c "^key2=value2[ \t\n\r]*$")
    ok(kv2.contains("/b/mn/z.c"));

    // new_ec_test(star_star_over_separator10 star_star.in bmnz.c "^[ \t\n\r]*$")
    ok(!kv1.contains("/bmnz.c"));
    ok(!kv2.contains("/bmnz.c"));
    ok(!kv3.contains("/bmnz.c"));
    ok(!kv4.contains("/bmnz.c"));

    // new_ec_test(star_star_over_separator11 star_star.in bm/nz.c "^[ \t\n\r]*$")
    ok(!kv1.contains("/bm/nz.c"));
    ok(!kv2.contains("/bm/nz.c"));
    ok(!kv3.contains("/bm/nz.c"));
    ok(!kv4.contains("/bm/nz.c"));

    // new_ec_test(star_star_over_separator12 star_star.in bmn/z.c "^[ \t\n\r]*$")
    ok(!kv1.contains("/bmn/z.c"));
    ok(!kv2.contains("/bmn/z.c"));
    ok(!kv3.contains("/bmn/z.c"));
    ok(!kv4.contains("/bmn/z.c"));

    // new_ec_test(star_star_over_separator13 star_star.in c/z.c "^key3=value3[ \t\n\r]*$")
    ok(kv3.contains("/c/z.c"));

    // new_ec_test(star_star_over_separator14 star_star.in cmn/z.c "^key3=value3[ \t\n\r]*$")
    ok(kv3.contains("/cmn/z.c"));

    // new_ec_test(star_star_over_separator15 star_star.in c/mn/z.c "^key3=value3[ \t\n\r]*$")
    ok(kv3.contains("/c/mn/z.c"));

    // new_ec_test(star_star_over_separator16 star_star.in cmnz.c "^[ \t\n\r]*$")
    ok(!kv1.contains("/cmnz.c"));
    ok(!kv2.contains("/cmnz.c"));
    ok(!kv3.contains("/cmnz.c"));
    ok(!kv4.contains("/cmnz.c"));

    // new_ec_test(star_star_over_separator17 star_star.in cm/nz.c "^[ \t\n\r]*$")
    ok(!kv1.contains("/cm/nz.c"));
    ok(!kv2.contains("/cm/nz.c"));
    ok(!kv3.contains("/cm/nz.c"));
    ok(!kv4.contains("/cm/nz.c"));

    // new_ec_test(star_star_over_separator18 star_star.in c/mnz.c "^[ \t\n\r]*$")
    ok(!kv1.contains("/c/mnz.c"));
    ok(!kv2.contains("/c/mnz.c"));
    ok(!kv3.contains("/c/mnz.c"));
    ok(!kv4.contains("/c/mnz.c"));

    // new_ec_test(star_star_over_separator19 star_star.in d/z.c "^key4=value4[ \t\n\r]*$")
    ok(kv4.contains("/d/z.c"));

    // new_ec_test(star_star_over_separator20 star_star.in d/mn/z.c "^key4=value4[ \t\n\r]*$")
    ok(kv4.contains("/d/mn/z.c"));

    // new_ec_test(star_star_over_separator21 star_star.in dmnz.c "^[ \t\n\r]*$")
    ok(!kv1.contains("/dmnz.c"));
    ok(!kv2.contains("/dmnz.c"));
    ok(!kv3.contains("/dmnz.c"));
    ok(!kv4.contains("/dmnz.c"));

    // new_ec_test(star_star_over_separator22 star_star.in dm/nz.c "^[ \t\n\r]*$")
    ok(!kv1.contains("/dm/nz.c"));
    ok(!kv2.contains("/dm/nz.c"));
    ok(!kv3.contains("/dm/nz.c"));
    ok(!kv4.contains("/dm/nz.c"));

    // new_ec_test(star_star_over_separator23 star_star.in d/mnz.c "^[ \t\n\r]*$")
    ok(!kv1.contains("/d/mnz.c"));
    ok(!kv2.contains("/d/mnz.c"));
    ok(!kv3.contains("/d/mnz.c"));
    ok(!kv4.contains("/d/mnz.c"));

    // new_ec_test(star_star_over_separator24 star_star.in dmn/z.c "^[ \t\n\r]*$")
    ok(!kv1.contains("/dmn/z.c"));
    ok(!kv2.contains("/dmn/z.c"));
    ok(!kv3.contains("/dmn/z.c"));
    ok(!kv4.contains("/dmn/z.c"));

} // test_core_globstar()

/// test EditorConfig files with UTF-8 characters larger than 127
void
test_core_utf8()
{
    // // --- utf8char.in ---
    // ; test EditorConfig files with UTF-8 characters larger than 127
    //
    // root = true
    //
    // [中文.txt]
    // key = value
    Matcher m({"中文.txt"}, "/");

    // new_ec_test(utf_8_char utf8char.in "中文.txt" "^key=value[ \t\n\r]*$")
    ok(m.contains("/中文.txt"));
} // test_core_utf8

// }}}1

/// Tests of multiple globs
void
test_multi_glob()
{
    Matcher m({"a", "{1..10}", "{foo,bar}", "b", "*.txt", "{20..30}"}, "/");
    ok(!m.contains("/_"));
    ok(m.contains("/a"));
    ok(!m.contains("/0"));
    ok(m.contains("/1"));
    ok(m.contains("/10"));
    ok(!m.contains("/11"));
    ok(m.contains("/foo"));
    ok(m.contains("/bar"));
    ok(!m.contains("/bat"));
    ok(m.contains("/b"));
    ok(m.contains("/bat.txt"));
    ok(m.contains("/.txt"));
    ok(!m.contains("/19"));
    ok(m.contains("/20"));
    ok(m.contains("/25"));
    ok(m.contains("/30"));
    ok(!m.contains("/31"));

    ok(!m.contains("/foo/_"));
    ok(m.contains("/foo/a"));
    ok(!m.contains("/foo/0"));
    ok(m.contains("/foo/1"));
    ok(m.contains("/foo/10"));
    ok(!m.contains("/foo/11"));
    ok(m.contains("/foo/foo"));
    ok(m.contains("/foo/bar"));
    ok(!m.contains("/foo/bat"));
    ok(m.contains("/foo/b"));
    ok(m.contains("/foo/bat.txt"));
    ok(m.contains("/foo/.txt"));
    ok(!m.contains("/foo/19"));
    ok(m.contains("/foo/20"));
    ok(m.contains("/foo/25"));
    ok(m.contains("/foo/30"));
    ok(!m.contains("/foo/31"));
}

int
main()
{
    TEST_CASE(test_empty);
    TEST_CASE(test_invalid);
    TEST_CASE(test_not_finalized);
    TEST_CASE(test_ec455);
    TEST_CASE(test_specialchar_dirname);

    TEST_CASE(test_exact_match);
    TEST_CASE(test_extension);
    TEST_CASE(test_extension_negpos);
    TEST_CASE(test_extension_posneg);
    TEST_CASE(test_namestart);

    TEST_CASE(test_path_namestart);

    TEST_CASE(test_core_star);
    TEST_CASE(test_core_question);
    TEST_CASE(test_core_brackets);
    TEST_CASE(test_core_braces);
    TEST_CASE(test_core_globstar);
    TEST_CASE(test_core_utf8);

    TEST_CASE(test_multi_glob);

    TEST_RETURN;
}

// === Additional documentation ========================================== {{{1

/**
 * @file t/globstari-globset-t.cpp
 * @copyright @parblock
#
# Copyright (c) 2011-2018 EditorConfig Team
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
 * @endparblock
 */

// }}}1
// vi: set fdm=marker fenc=utf-8: //
