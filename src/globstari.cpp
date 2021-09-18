/// @file src/globstari.cpp
/// @brief globstar + ignore routines --- GlobSet implementation.
/// @details Part of smallcxx
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White
/// @copyright Uses some code from editorconfig-core-c, licensed BSD.
///     See below for the editorconfig-core-c copyright statement.
/// @todo permit matching only directories (trailing slash on globs)

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include <ctype.h>
#include <istream>
#include <list>
#include <sstream>
#include <stdexcept>
#include <string.h>
#include <unordered_set>

#include "smallcxx/globstari.hpp"
#include "smallcxx/logging.hpp"
#include "smallcxx/string.hpp"

using namespace std;

namespace smallcxx
{

// === Internal types and constants ======================================

namespace glob
{

/// Characters that are special in globs so should be escaped.
/// @details from editorconfig-core-c/src/lib/ec_glob.c
extern const string ec_special_chars = "?[]\\*-{},";

// --- PCRE2-related types -----------------------------------------------

using MatchDataPtr =
    std::unique_ptr<pcre2_match_data, void(*)(pcre2_match_data *)>;

/// @todo Make this a std::shared_ptr?
using RePtr = std::unique_ptr<pcre2_code, void(*)(pcre2_code *)>;

// --- Misc. types -------------------------------------------------------

/// Element of a numerical range
using Int = intmax_t;

/// Numerical range
using IntPair = std::pair<Int, Int>;

/// List of numerical ranges
using RangePairs = std::vector<IntPair>;

// --- Criteria class (regex + range(s)) ---------------------------------

/// Regex and ranges for >=1 globs.
class Criteria
{
    RePtr compiled_;
    RangePairs ranges_;  ///< may be empty

public:

    Criteria(): compiled_(nullptr, pcre2_code_free) {};
    Criteria(const Criteria& other)
        : compiled_(nullptr, pcre2_code_free)
        , ranges_(other.ranges_)
    {
        if(other.compiled_) {
            compiled_.reset(pcre2_code_copy_with_tables(other.compiled_.get()));
        }
    }
    ~Criteria() = default;

    Criteria(const std::string& reSrc): Criteria(reSrc, {}) {}

    Criteria(const std::string& reSrc, const RangePairs& ranges)
        : compiled_(nullptr, pcre2_code_free)
        , ranges_(ranges)
    {
        LOG_F(LOG, "RE >>%s<< with %zu ranges", reSrc.c_str(), ranges_.size());

        int error_code;
        size_t erroffset;
        pcre2_code *re = pcre2_compile(
                             (PCRE2_SPTR8)reSrc.c_str(), PCRE2_ZERO_TERMINATED, 0,
                             &error_code, &erroffset,
                             nullptr);

        if (!re) {      /* failed to compile */
            throw runtime_error(STR_OF << "Could not compile regex "
                                << ">>" << reSrc << "<<: error " << error_code
                                << " at offset " << erroffset);
        }

        compiled_.reset(re);
        re = nullptr;
    } // Criteria(string, RangePairs)

    /// Does @p str match compiled_?
    bool accepts(const smallcxx::glob::Path& str) const;

}; // class Criteria

/// @details Some code from editorconfig-core-c/src/lib/ec_glob.c:ec_glob()
bool
Criteria::accepts(const smallcxx::glob::Path& str) const
{
    MatchDataPtr matches(
        pcre2_match_data_create_from_pattern(compiled_.get(), nullptr),
        pcre2_match_data_free);

    int rc;
    size_t *pcre_result;

    rc = pcre2_match(compiled_.get(), (PCRE2_SPTR8)str.c_str(), str.length(),
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
              str.c_str());
        return false;
    }

    /* Check whether the numbers are in the desired range */
    size_t i;
    auto rangeit = ranges_.cbegin();
    for(i = 1; rangeit != ranges_.cend(); ++i, ++rangeit) {

        const auto ofsBegin = pcre_result[2 * i];
        const auto ofsEnd = pcre_result[2 * i + 1];

        // Didn't match this capturing group --- keep going
        if(ofsBegin == PCRE2_UNSET && ofsEnd == PCRE2_UNSET) {
            continue;
        }

        const char *substring_start = str.data() + pcre_result[2 * i];
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
}

// --- Glob -> Regex conversion ------------------------------------------ {{{1

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
                    is_single = 0;  // XXX is this unreachable?
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

// }}}1
// --- GlobSetImpl -------------------------------------------------------

class GlobSetImpl
{
    PathSet globs_;             ///< individual globs (input)

    /// What we match
    std::list<Criteria> criteria_;

    bool finalized_ = false;

public:

    /// Add a single glob to the set.
    void addGlob(const smallcxx::glob::Path& glob);

    /// Implementation of GlobSet::Finalize().
    /// @details Fill in criteria_ from globs_.
    /// @throws std::runtime_error if any regex cannot be constructed.
    /// @note Calling finalize() without first adding any globs is not
    ///     an error!  It will give you a GlobSet that matches nothing.
    void finalize();

    /// Whether finalize() has been called
    bool
    finalized() const
    {
        return finalized_;
    }

    /// Implementation of GlobSet::contains().
    bool contains(const smallcxx::glob::Path& path) const;

}; // class GlobSetImpl

void
GlobSetImpl::addGlob(const smallcxx::glob::Path& glob)
{
    if(glob.empty()) {
        throw runtime_error("Cannot add an empty glob");
    }
    if(finalized()) {
        throw runtime_error("Already finalized --- cannot add more globs");
    }

    globs_.insert(glob);
} // GlobSetImpl::addGlob()

void
GlobSetImpl::finalize()
{
    // regex for all the globs that don't have numerical ranges
    string nonRangeSrc("^(?:");
    bool hasNonRange = false;

    for(const auto& glob : globs_) {
        string reSrc;
        RangePairs ranges;

        globToRegexSrc(glob, reSrc, ranges);

        if(ranges.empty()) {
            // No numeric groups --- can be combined with all other globs
            // having no numeric groups

            hasNonRange = true;

            // atomic group --- each individual branch of the glob
            // will either match or it won't.
            nonRangeSrc += "(?>";
            nonRangeSrc += reSrc;
            nonRangeSrc += ")|";

        } else {
            // Has numeric groups --- must be evaluated on its own.
            // This is because otherwise all numeric values textually match
            // at the first numeric-group placeholder in a regex, so we
            // can't get past the first option.  E.g.,
            //
            // Globs: {1..10}, {101..110}
            // Regex (simplified): /^(?:(\d+)|(\d+))$/
            // The match never gets to $2, because digits are caught by $1.

            reSrc = "^(?>" + reSrc + ")$";
            criteria_.emplace_back(reSrc, ranges);
        }

    } // foreach glob

    if(hasNonRange) {
        nonRangeSrc += "(*FAIL)";   // need something after the last `|`
        nonRangeSrc += ")$";

        // Check the non-range ones first since we can dispose of a bunch
        // in a single check.  Also, my experience is that globs without
        // numeric ranges will be much more common than globs with
        // numeric ranges.
        criteria_.emplace_front(nonRangeSrc, RangePairs{});
    }

    finalized_ = true;
} // GlobSetImpl::finalize()

/// @details Adapted from editorconfig-core-c/src/lib/ec_glob.c:ec_glob(),
/// the second half of the function.
bool
GlobSetImpl::contains(const smallcxx::glob::Path& path) const
{
    if(!finalized()) {
        throw logic_error("Glob set was not finalized");
    }

    for(const auto& criteria : criteria_) {
        if(criteria.accepts(path)) {
            return true;
        }
    }

    return false;
} // GlobSetImpl::contains()

// --- GlobSet -----------------------------------------------------------

GlobSet::GlobSet()
    : impl_(new GlobSetImpl())
{}

GlobSet::GlobSet(const GlobSet& other)
    : impl_(new GlobSetImpl(*other.impl_.get()))
{}

/// dtor.  Must be expressly declared so impl_'s deleter is called
/// at a point where the definition of GlobSetImpl is available.
GlobSet::~GlobSet()
{}

void
GlobSet::addGlob(const smallcxx::glob::Path& glob)
{
    impl_->addGlob(glob);
}

void
GlobSet::finalize()
{
    impl_->finalize();
}

bool
GlobSet::finalized() const
{
    return impl_->finalized();
}
bool
GlobSet::contains(const smallcxx::glob::Path& path) const
{
    return impl_->contains(path);
}

} // namespace glob

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

// vi: set fdm=marker: //
