/// @file src/globstari.cpp
/// @brief globstar + ignore routines --- implementation.
/// @details Part of smallcxx
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White
/// @copyright Uses some code from editorconfig-core-c, licensed BSD.
///     See below for the editorconfig-core-c copyright statement.
/// @todo permit matching only directories (trailing slash on globs)

#include <ctype.h>
#include <stdexcept>
#include <string.h>

#include "smallcxx/globstari.hpp"
#include "smallcxx/logging.hpp"
#include "smallcxx/string.hpp"

using namespace std;
using smallcxx::glob::Path;
using smallcxx::glob::PathCheckResult;

namespace smallcxx
{

// === Internal classes ==================================================

namespace detail
{

/// Control exception used to stop traversal early
struct StopTraversal {};

/// Characters that are special in globs so should be escaped.
/// @details from editorconfig-core-c/src/lib/ec_glob.c
const string ec_special_chars = "?[]\\*-{},";

} // namespace detail

namespace glob
{

// --- GlobSet -----------------------------------------------------------

GlobSet::GlobSet()
    : compiled_(nullptr, pcre2_code_free) {}

GlobSet::GlobSet(const GlobSet& other)
    : compiled_(nullptr, pcre2_code_free)
{
    globs_ = other.globs_;
    ranges_ = other.ranges_;

    if(other.compiled_) {
        compiled_.reset(pcre2_code_copy_with_tables(other.compiled_.get()));
    }
}

void
GlobSet::addGlob(const smallcxx::glob::Path& glob)
{
    if(glob.empty()) {
        throw runtime_error("Cannot add an empty glob");
    }
    globs_.insert(glob);
}

/// Append regex source for glob @p glob to @p src, and append to @p ranges
/// if @p glob includes numerical range(s).
/// @details Adapted from editorconfig-core-c/src/lib/ec_glob.c:ec_glob(),
/// the first half of the function.
static void
globToRegexSrc(const smallcxx::glob::Path& glob, string& src,
               RangePairs& ranges)
{
    // Places we need to force in a backslash.
    // Invariant: the end of the string can't be used.
    unordered_set<const char *> toBackslash;

    const char *c;
    int brace_level = 0;
    bool is_in_bracket = false;
    int error_code;
    size_t erroffset;
    pcre2_code *reNumRaw;
    int rc;
    bool are_braces_paired = true;

    /* Determine whether curly braces are paired */
    {
        int     left_count = 0;
        int     right_count = 0;
        for (c = &glob[0]; *c; ++ c) {
            if (*c == '\\' && *(c + 1) != '\0') {
                ++ c;
                continue;
            }

            if (*c == '}') {
                ++ right_count;
            } else if (*c == '{') {
                ++ left_count;
            }

            if (right_count > left_count) {
                are_braces_paired = false;
                break;
            }
        }

        if (right_count != left_count) {
            are_braces_paired = false;
        }
    }

    /* used to search for {num1..num2} case */
    // TODO make this a singleton
    reNumRaw = pcre2_compile((PCRE2_SPTR8)
                             "^\\{[\\+\\-]?\\d+\\.\\.[\\+\\-]?\\d+\\}$",
                             PCRE2_ZERO_TERMINATED, 0,
                             &error_code, &erroffset, nullptr);
    if (!reNumRaw) {      /* failed to compile */
        throw std::runtime_error(STR_OF << "Could not create reNum: "
                                 << error_code << " at offset " << erroffset);
    }
    RePtr pReNum(reNumRaw, pcre2_code_free);
    reNumRaw = nullptr;

    for (c = &glob[0]; *c; ++ c) {

        // Force in backslashes
        if(toBackslash.find(c) != toBackslash.end()) {
            if (*c != '\0') {
                src += '\\';
                src += *c;
            } else {
                throw logic_error("toBackslash invariant violation");
            }
            continue;
        }

        switch (*c) {
        case '\\':      /* also skip the next one */
            if (*(c + 1) != '\0') {
                src += *(c++);
                src += *c;
            } else {
                src += "\\\\";
            }

            break;
        case '?':
            src += "[^/]";
            break;
        case '*':
            if (*(c + 1) == '*') {  /* case of ** */
                src += ".*";
                ++ c;
            } else {                /* case of * */
                src += "[^\\/]*";
            }

            break;
        case '[':
            if (is_in_bracket) {   /* inside brackets, we really mean bracket */
                src += "\\[";
                break;
            }

            {
                /* check whether we have slash within the bracket */
                bool has_slash = false;
                const char *cc;
                for (cc = c; *cc && *cc != ']'; ++ cc) {
                    if (*cc == '\\' && *(cc + 1) != '\0') {
                        ++ cc;
                        continue;
                    }

                    if (*cc == '/') {
                        has_slash = 1;
                        break;
                    }
                }

                /* if we have slash in the brackets, just do it literally */
                if (has_slash) {
                    const char *right_bracket = strchr(c, ']');

                    if (!right_bracket) { /* The right bracket may not exist */
                        right_bracket = c + strlen(c);
                    }

                    src += "\\";
                    copy(c, right_bracket, back_inserter(src));
                    if (*right_bracket) { /* right_bracket is a bracket, not \0 */
                        src += "\\]";
                    }
                    c = right_bracket;
                    if (!*c)
                        /* end of string, meaning that right_bracket is not a
                         * bracket. Then we go back one character to make the
                         * parsing end normally for the counter in the "for"
                         * loop. */
                    {
                        c -= 1;
                    }
                    break;
                }
            }

            is_in_bracket = 1;
            if (*(c + 1) == '!') { /* case of [!...] */
                src += "[^";
                ++ c;
            } else {
                src += '[';
            }

            break;

        case ']':
            is_in_bracket = 0;
            src += *c;
            break;

        case '-':
            if (is_in_bracket) {    /* in brackets, - indicates range */
                src += *c;
            } else {
                src += "\\-";
            }

            break;
        case '{':
            if (!are_braces_paired) {
                src += "\\{";
                break;
            }

            /* Check the case of {single}, where single can be empty */
            {
                const char *cc;
                bool is_single = true;

                for (cc = c + 1; *cc != '\0' && *cc != '}'; ++ cc) {
                    if (*cc == '\\' && *(cc + 1) != '\0') {
                        ++ cc;
                        continue;
                    }

                    if (*cc == ',') {
                        is_single = 0;
                        break;
                    }
                } // for cc

                if (*cc == '\0') {
                    is_single = 0;
                }

                if (is_single) {    /* escape the { and the corresponding } */
                    // A {} block with no commas in it
                    const char *double_dots;
                    IntPair intpair;

                    pcre2_match_data   *match_data = pcre2_match_data_create_from_pattern(
                                                         pReNum.get(), nullptr);

                    /* Check the case of {num1..num2} */
                    // c points to the `{`.  cc points to the `}`.
                    rc = pcre2_match(pReNum.get(),
                                     (PCRE2_SPTR8)c, cc - c + 1, 0, 0, match_data,
                                     nullptr);

                    pcre2_match_data_free(match_data);

                    if (rc < 0) {  /* not {num1..num2} case */
                        src += "\\{";

                        // Remember that, when c gets to where cc is now, we
                        // need to insert a \\ just before the rbrace.
                        toBackslash.insert(cc);

                        break;
                    }

                    /* Get the range */
                    double_dots = strstr(c, "..");
                    intpair.first = strtoll(c + 1, nullptr, 10);
                    intpair.second = strtoll(double_dots + 2, nullptr, 10);

                    ranges.push_back(intpair);

                    src += "([\\+\\-]?\\d+)";
                    c = cc;

                    break;
                } // endif is_single
            } // end anonymous scope

            ++ brace_level;
            src += "(?:";
            break;

        case '}':
            if (!are_braces_paired) {
                src += "\\}";
                break;
            }

            -- brace_level;
            src += ')';
            break;

        case ',':
            if (brace_level > 0) { /* , inside {...} */
                src += '|';
            } else {
                src += "\\,";
            }
            break;

        case '/':
            // /**/ case, match both single / and /anything/
            if (!strncmp(c, "/**/", 4)) {
                src += "(\\/|\\/.*\\/)";
                c += 3;
            } else {
                src += "\\/";
            }

            break;

        default:
            if (!isalnum(*c)) {
                src += '\\';
            }

            src += *c;
        } // switch(*c)
    } // for c

} // globToRegexSrc

/// @details Some code from editorconfig-core-c/src/lib/ec_glob.c:ec_glob()
void
GlobSet::finalize()
{
    int error_code;
    size_t erroffset;
    pcre2_code *re = nullptr;

    string src("^(?:");

    if(globs_.empty()) {
        src += "(*FAIL)";
    }

    bool first = true;
    for(const auto& glob : globs_) {
        if(first) {
            first = false;
        } else {
            src += "|";
        }

        src += "(?:";
        globToRegexSrc(glob, src, ranges_);
        src += ")";
    } // foreach glob

    src += ")$";

    LOG_F(LOG, "RE is >>%s<< with %zu ranges", src.c_str(), ranges_.size());

    re = pcre2_compile((PCRE2_SPTR8)src.c_str(), PCRE2_ZERO_TERMINATED, 0,
                       &error_code, &erroffset,
                       nullptr);

    if (!re) {      /* failed to compile */
        throw runtime_error(STR_OF << "Could not compile regex "
                            << "<<" << src << ">>: error " << error_code
                            << " at offset " << erroffset);
    }

    compiled_ = RePtr(re, pcre2_code_free);
}

/// @details Adapted from editorconfig-core-c/src/lib/ec_glob.c:ec_glob(),
/// the second half of the function.
bool
GlobSet::contains(const smallcxx::glob::Path& path) const
{
    if(!finalized()) {
        throw logic_error("Glob set was not finalized");
    }

    MatchDataPtr matches(
        pcre2_match_data_create_from_pattern(compiled_.get(), nullptr),
        pcre2_match_data_free);

    int rc;
    size_t *pcre_result;

    rc = pcre2_match(compiled_.get(), (PCRE2_SPTR8)path.c_str(), path.length(),
                     0, 0, matches.get(), nullptr);

    if (rc < 0) {   /* failed to match */
        if (rc == PCRE2_ERROR_NOMATCH) {
            return false;
        } else {
            throw runtime_error(STR_OF
                                << "Failure while matching RE: code " << rc);
        }
    }

    pcre_result = pcre2_get_ovector_pointer(matches.get());

    // Did anything match?
    if((pcre_result[1] == 0 ) ||
            (pcre_result[0] == PCRE2_UNSET && pcre_result[1] == PCRE2_UNSET)) {
        LOG_F(FIXME, "Zero-length successful match --- probably a bug!  >>%s<<",
              path.c_str());
        return false;
    }

    /* Check whether the numbers are in the desired range */
    size_t i;
    // TODO flip this around? --- only check the ranges that actually matched?
    auto rangeit = ranges_.cbegin();
    for(i = 1; rangeit != ranges_.cend(); ++ i, ++rangeit) {

        const auto ofsBegin = pcre_result[2 * i];
        const auto ofsEnd = pcre_result[2 * i + 1];

        // Didn't match this capturing group --- keep going
        if(ofsBegin == PCRE2_UNSET && ofsEnd == PCRE2_UNSET) {
            continue;
        }

        const char *substring_start = path.data() + pcre_result[2 * i];
        size_t substring_length = pcre_result[2 * i + 1] - pcre_result[2 * i];

        /* we don't consider 0 digits such as 010 as matched */
        if (*substring_start == '0') {
            return false;
        }

        if(substring_length == 0) {
            // XXX is this the right thing?
            throw runtime_error(STR_OF << "Zero length substring match at"
                                << " index " << i);
        }

        string num_string(substring_start, substring_length);
        Int num = strtoll(num_string.c_str(), nullptr, 10);

        if (num < rangeit->first || num > rangeit->second) { /* not matched */
            return false;   // it has to match all of them
        }
    } // for each range

    return true;

} // GlobSet::contains()

// --- Matcher: Initializing ---------------------------------------------

Matcher::Matcher(const std::initializer_list<smallcxx::glob::Path>& globs,
                 const smallcxx::glob::Path path)
{
    for(const auto& g : globs) {
        addGlob(g, path);
    }
    finalize();
}

void
Matcher::addGlob(const smallcxx::glob::Path& glob)
{
    if(glob.empty()) {
        throw runtime_error("Cannot add an empty glob");
    }

    Polarity polarity = (glob[0] == '!') ? Polarity::Exclude : Polarity::Include;
    SetAndPolarity sp(polarity);

    if(globsets_.empty()) {
        globsets_.push_back(sp);

    } else if(globsets_.back().polarity != polarity) {
        globsets_.back().globSet.finalize();
        globsets_.push_back(sp);
    }

    if(glob[0] == '!') {
        globsets_.back().globSet.addGlob(&glob[1]);     // strip leading `!`
    } else {
        globsets_.back().globSet.addGlob(glob);
    }
}

/// @details Adapted from
/// editorconfig-core-c/src/lib/editorconfig.c:ini_handler()
void
Matcher::addGlob(const smallcxx::glob::Path& glob,
                 const smallcxx::glob::Path& path)
{

    if(path.empty() || *path.crbegin() != '/') {
        throw domain_error("Matcher::addGlob: path must be nonempty and end with /");
    }

    // Strip trailing slash.  TODO handle this in a cleaner way.
    const auto pathNoSlash = path.substr(0, path.size() - 1);

    Polarity polarity = (glob[0] == '!') ? Polarity::Exclude : Polarity::Include;

    string fullGlob; // new glob of (essentially) `path`/`glob`

    /* fullGlob is:
     * - path[double_star]/[glob] if glob does not contain '/'
     * - path[glob] if glob starts with a '/'
     * - path/[glob] if glob contains '/' but does not start with '/'.
     *
     * If the path part has any special characters as defined by ec_glob.c, we
     * need to escape them.
     */

    /* Escaping special characters in the directory part. */
    size_t lastpos = 0;
    size_t pos;
    while((pos = pathNoSlash.find_first_of(detail::ec_special_chars,
                                           lastpos)) != pathNoSlash.npos) {
        fullGlob += pathNoSlash.substr(lastpos, pos - lastpos);
        fullGlob += '\\';  /* escaping char */
        fullGlob += pathNoSlash[pos];
        lastpos = pos + 1;
    }

    // Copy any of the string that may be left
    fullGlob += pathNoSlash.substr(lastpos);

    if (glob.find('/') == glob.npos) { // No / is found, append '[star][star]/'
        fullGlob += "**/";

    } else if (glob[0] != '/') {
        // The first char is not '/' but section contains '/', append a '/'
        fullGlob += "/";
    }

    if(polarity == Polarity::Include) {
        fullGlob += glob;
    } else {
        // Move the polarity `!` to the beginning of fullGlob
        fullGlob += &glob[1];
        fullGlob = '!' + fullGlob;
    }

    LOG_F(TRACE, "Glob '%s', path '%s', pathNoSlash '%s', fullGlob '%s'",
          glob.c_str(), path.c_str(), pathNoSlash.c_str(), fullGlob.c_str());

    addGlob(fullGlob);
} // Matcher::addGlob()

void
Matcher::finalize()
{
    if(!globsets_.empty()) {
        globsets_.back().globSet.finalize();
    }
}

bool
Matcher::ready() const
{
    return globsets_.empty() || globsets_.back().globSet.finalized();
}

// --- Matcher: Searching ------------------------------------------------

bool
Matcher::contains(const smallcxx::glob::Path& path) const
{
    if(!ready()) {
        throw logic_error("Call to Matcher::contains() when not ready --- call finalize() after adding globsets");
    }

    if(path.empty()) {
        return false;
    }

    if(*path.cbegin() != '/') {
        throw domain_error(
            "Matcher::contains: path must be absolute (start with /)");
    }

    // Check the globsets from back to front because later entries override
    // earlier entries.
    for(auto it = globsets_.crbegin(); it != globsets_.crend(); ++it) {
        if(it->globSet.contains(path)) {
            return (it->polarity == Polarity::Include);
        }
    }
    return false;
} // Matcher::contains()

PathCheckResult
Matcher::check(const smallcxx::glob::Path& path) const
{
    if(!ready()) {
        throw logic_error("Call to check() when not ready --- call finalize() after adding globsets");
    }

    if(globsets_.empty()) {
        return PathCheckResult::Unknown;
    }

    // Check the globsets from back to front because later entries override
    // earlier entries.
    for(auto it = globsets_.crbegin(); it != globsets_.crend(); ++it) {
        if(it->globSet.contains(path)) {
            return (it->polarity == Polarity::Include) ?
                   PathCheckResult::Included : PathCheckResult::Excluded;
        }
    }

    return PathCheckResult::Unknown;

} // Matcher::check()

} // namespace smallcxx::glob

// === GlobstariBase =====================================================

/// Split a path on the last `/`.  If no `/`, assume the whole thing is a name.
/// If @p path has a `/`, puts everything up to and including that `/` in
/// @p dir, and the rest in @p name.
static void
splitPath(const glob::Path& path, glob::Path& dir, glob::Path& name)
{
    const auto lastSlashPos = path.find_last_of("/");
    if(lastSlashPos == path.npos) {
        name = path;
        dir = "";
    } else {
        dir = path.substr(0, lastSlashPos + 1); // include the trailing slash
        name = path.substr(lastSlashPos + 1);
    }
}

/// @details Currently BFS, just because that's what I prefer!
void
GlobstariBase::traverse(const smallcxx::glob::Path& basePath,
                        const std::vector<smallcxx::glob::Path>& needle,
                        ssize_t maxDepth)
{
    LOG_F(FIXME, "This is a work in progress");

    Path rootPath = canonicalize(basePath);
    glob::Matcher needleMatcher;
    needleMatcher.addGlobs(needle, basePath);
    string dir, name;

    std::deque<Entry> entries;  // work queue
    splitPath(rootPath, dir, name);
    entries.emplace_back(EntryType::StartDir, rootPath, 0);

    try {
        traverseWorker(entries, rootPath, needleMatcher, maxDepth);
    } catch(detail::StopTraversal&) {
        // Nothing to do
    } // other exceptions propagate out

} // traverse()

/// Human-readable PathCheckResultNames.  All the same width to make the
/// logs easier to read.
static const char *PathCheckResultNames[3] = {
    "included",
    "excluded",
    "unknown ",
};

void
GlobstariBase::traverseWorker(
    std::deque<Entry>& entries, const smallcxx::glob::Path rootPath,
    const smallcxx::glob::Matcher& needleMatcher,
    ssize_t maxDepth)
{
    // Main work loop
    while(!entries.empty()) {
        const auto& entry = entries.front();
        entries.pop_front();

        if((maxDepth > 0) && (entry.depth > maxDepth)) {
            LOG_F(TRACE, "Skipping %s --- maxDepth exceeded",
                  entry.canonPath.c_str());
            return; // *** EXIT POINT ***
        }


        PathCheckResult match = PathCheckResult::Unknown;

        bool shouldProcess = false; // whether to call processEntry

        // TODO? figure out how to not special-case StartDir?
        if(entry.ty == EntryType::StartDir) {
            shouldProcess = true;
        } else {

            // TODO check ignores

            match = needleMatcher.check(entry.canonPath);
            shouldProcess = (match == PathCheckResult::Included);
        }

        LOG_F(TRACE, "%s %s", PathCheckResultNames[(int)match],
              entry.canonPath.c_str());

        if(!shouldProcess) {
            continue;
        }

        const auto status = processEntry(entry);

        switch(status) {
        case ProcessStatus::Continue:
            if(entry.ty == EntryType::StartDir || entry.ty == EntryType::Dir) {
                auto newEntries = readDir(entry.canonPath);
                for(auto& newEntry : newEntries) {
                    newEntry.depth = entry.depth + 1;
                }
                move(newEntries.begin(), newEntries.end(), back_inserter(entries));
            }
            break;

        case ProcessStatus::Skip:
            // nothing to do
            break;

#if 0
        // TODO implement this?
        case ProcessStatus::Pop:
            // TODO pop the deque until `dir` changes
            break;
#endif

        case ProcessStatus::Stop:
            throw detail::StopTraversal();
            break;

        default:
            throw logic_error(STR_OF << "Unimplemented status value "
                              << (int)status);
            break;
        }

    } // while entries remain
} // GlobstariBase::traverseWorker

#if 0
void
GlobstariBase::traverseDir(const smallcxx::glob::Path& dirName,
                           const glob::Matcher& needle,
                           ssize_t maxDepth, ssize_t depth)
{
    if((maxDepth > 0) && (depth > maxDepth)) {
        LOG_F(TRACE, "Skipping %s --- maxDepth exceeded", dirName.c_str());
        return; // *** EXIT POINT ***
    }

    // TODO get ignores w.r.t. @p dirName

    const auto& entries = readDir(dirName);
    for(const auto& entry : entries) {

        // TODO check ignores

        const auto match = needle.check(entry.canonPath);
        if(match == PathCheckResult::Excluded) {
            LOG_F(TRACE, "excluded: %s", canonPath.c_str());
            continue;
        }

        if(!needle.contains(canonPath)) {
            LOG_F(TRACE, "not a needle: %s", canonPath.c_str());
            continue;
        }

        const auto status = processEntry(entry);

        switch(status) {
        case ProcessStatus::Continue:
            break;
        case ProcessStatus::Pop:
            goto loopExit;
            break;
        case ProcessStatus::Stop:
            throw detail::StopTraversal();
            break;
        default:
            LOG_F(FIXME, "Unimplemented status value %d", (int)status);
            throw logic_error("Unimplemented status value");
            break;
        }

    } //foreach entry
loopExit:

    NOTHING;
} // traverseDir()
#endif

// === GlobstariDisk =====================================================

std::vector<GlobstariBase::Entry>
GlobstariDisk::readDir(const Path& dirName)
{
    LOG_F(FIXME, "not yet implemented");
    return {};
}

bool
GlobstariDisk::readFile(const Path& dirPath, const Path& name,
                        Bytes& contents)
{
    LOG_F(FIXME, "not yet implemented");
    return false;
}

smallcxx::glob::Path
GlobstariDisk::canonicalize(const Path& path)
{
    LOG_F(FIXME, "not yet implemented");
    return "";
}

} // namespace smallcxx

/**
 * @file src/globstari.cpp
 * @copyright @parblock
 * Editorconfig-core-c copyright from ec_glob.c:
 *
 * Copyright (c) 2014-2019 Hong Xu <hong AT topbug DOT net>
 *
 * Copyright (c) 2018 Sven Strickroth <email AT cs-ware DOT de>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * @endparblock
 */
