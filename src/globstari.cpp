/// @file src/globstari.cpp
/// @brief globstar + ignore routines --- implementation.
/// @details Part of smallcxx
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White
/// @copyright Uses some code from editorconfig-core-c, licensed BSD.
///     See this source file for the copyright statement.

#define PCRE2_CODE_UNIT_WIDTH 8

#include <functional>
#include <memory>
#include <pcre2.h>
#include <stdexcept>
#include <unordered_set>

#include "smallcxx/globstari.hpp"
#include "smallcxx/logging.hpp"

using namespace std;

namespace smallcxx
{

// === Internal classes ==================================================

namespace detail
{

/// Control exception used to stop traversal early
struct StopTraversal {};

// --- GlobSet -----------------------------------------------------------

/// Set of globs (convenience typedef)
using PathSet = std::unordered_set<GlobstariBase::Path>;

/// List of globs (convenience typedef)
using Paths = std::vector<GlobstariBase::Path>;

/// Polarity of globs: include or exclude
enum class Polarity {
    INCLUDE,   ///< includes (don't start with `!`)
    EXCLUDE,   ///< excludes (start with `!`)
};

/// A set of globs of the same polarity
class GlobSet
{
    const Polarity polarity_;   ///< polarity
    PathSet globs_;             ///< individual globs

    /// regex matching everything in globs_
    using ReType = unique_ptr<pcre2_code, std::function<void(pcre2_code*)> > ;
    ReType re_;

    /// Numeric ranges for capture groups listed in re_
    vector<pair<intmax_t, intmax_t> > ranges_;

public:
    /// Make a set with the given polarity
    GlobSet(Polarity polarity): polarity_(polarity) {}

    /// Our polarity
    Polarity
    polarity() const
    {
        return polarity_;
    }

    void
    addGlob(const GlobstariBase::Path& glob)  ///< Add a single glob to the set
    {
        globs_.insert(glob);
    }

    void finalize();                ///< fill in re_ from globs_

    /// Whether finalize() has been called
    bool
    finalized() const
    {
        return (bool)re_;
    }

    /// Returns true if the GlobSet contains @p path.
    /// @throws logic_error if finalize() has not been called.
    bool contains(const GlobstariBase::Path& path) const;
}; // class GlobSet

/// @details Adapted from editorconfig-core-c/src/lib/ec_glob.c:ec_glob()
void
GlobSet::finalize()
{
    // TODO build re_ and fill in ranges_
    pcre2_code *re = nullptr;
    re_ = ReType(re, pcre2_code_free);
}

/// @details Adapted from editorconfig-core-c/src/lib/ec_glob.c:ec_glob()
bool
GlobSet::contains(const GlobstariBase::Path& path) const
{
    if(!re_) {
        throw logic_error("Glob set was not finalized");
    }

    std::unique_ptr<pcre2_match_data, std::function<void(pcre2_match_data*)> >
    matches(pcre2_match_data_create_from_pattern(re_.get(), NULL),
            pcre2_match_data_free);

    return false;

    // TODO
#if 0
    rc = pcre2_match(re_.get(), string, strlen(string), 0, 0, pcre_match_data,
                     NULL);

    if (rc < 0) {   /* failed to match */
        if (rc == PCRE2_ERROR_NOMATCH) {
            ret = EC_GLOB_NOMATCH;
        } else {
            ret = rc;
        }

        goto cleanup;
    }

    /* Whether the numbers are in the desired range? */
    pcre_result = pcre2_get_ovector_pointer(pcre_match_data);
    for(p = (int_pair *) utarray_front(nums), i = 1; p;
            ++ i, p = (int_pair *) utarray_next(nums, p)) {
        const char * substring_start = string + pcre_result[2 * i];
        size_t  substring_length = pcre_result[2 * i + 1] - pcre_result[2 * i];
        char *       num_string;
        int          num;

        /* we don't consider 0digits such as 010 as matched */
        if (*substring_start == '0') {
            break;
        }

        num_string = strndup(substring_start, substring_length);
        if (num_string == NULL) {
            ret = -2;
            goto cleanup;
        }
        num = ec_atoi(num_string);
        free(num_string);

        if (num < p->num1 || num > p->num2) { /* not matched */
            break;
        }
    }

    if (p != NULL) {    /* numbers not matched */
        ret = EC_GLOB_NOMATCH;
    }

cleanup:

    pcre2_code_free(re);
    utarray_free(nums);

#endif
}

// --- Matcher -----------------------------------------------------------

/// Matcher built iteratively from any number of glob patterns.
///
/// The patterns are grouped into sets, each glob in a set having
/// the same Polarity.  For example,
/// ```
/// *.bak
/// *.swp
/// !*.foo
/// *.bar
/// ```
/// will have, in order, a Polarity::INCLUDE set matching `*.{bak,swp}`;
/// a Polarity::EXCLUDE set matching `*.foo`; and a Polarity::INCLUDE set
/// matching `*.bar`.
class Matcher
{
private:

    /// The globs we match (or don't)
    vector<GlobSet> globsets_;

public:

    /// Add a single glob to the matcher
    void addGlob(const GlobstariBase::Path& glob);

    /// Add multiple globs to the matcher
    void
    addGlobs(const Paths& globs)
    {
        for(const auto& p : globs) {
            addGlob(p);
        }
    }

    /// Call this once all the globs have been added
    void
    finalize()
    {
        if(!globsets_.empty()) {
            globsets_.back().finalize();
        }
    }

    /// Check whether the Matcher contains @p path.
    /// @throws logic_error if the finalize() has not been called.
    /// @returns true if @p path is included; false if @p is excluded
    ///     or if there are no globsets in the Matcher.
    bool
    contains(const GlobstariBase::Path& path)
    {
        if(!globsets_.empty() && !globsets_.back().finalized()) {
            throw logic_error("Call to contains() before finalizing");
        }

        for(const auto& globset : globsets_) {
            if(globset.contains(path)) {
                return (globset.polarity() == Polarity::INCLUDE);
            }
        }
        return false;
    }

}; // class Matcher

void
Matcher::addGlob(const GlobstariBase::Path& glob)
{
    if(glob.empty()) {
        throw runtime_error("Cannot add an empty glob");
    }

    Polarity polarity = (glob[0] == '!') ? Polarity::EXCLUDE : Polarity::INCLUDE;
    if(globsets_.empty()) {
        globsets_.emplace_back(polarity);
    } else if(globsets_.back().polarity() != polarity) {
        globsets_.back().finalize();
        globsets_.emplace_back(polarity);
    }

    globsets_.back().addGlob(glob);
}

} // namespace smallcxx::detail

// === GlobstariBase =====================================================

void
GlobstariBase::traverse(const GlobstariBase::Path& basePath,
                        const std::vector<GlobstariBase::Path>& globs,
                        ssize_t maxDepth)
{
    LOG_F(FIXME, "not yet implemented");
    try {
        traverseDir(basePath, globs, maxDepth, 0);
    } catch(detail::StopTraversal&) {
        // Nothing to do
    } // other exceptions propagate out
} // traverse()

void
GlobstariBase::traverseDir(const GlobstariBase::Path& dirName,
                           const std::vector<GlobstariBase::Path>& globs,
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

        // TODO check @p globs

        const auto status = processEntry(entry);

        switch(status) {
        case ProcessStatus::CONTINUE:
            break;
        case ProcessStatus::POP:
            goto loopExit;
            break;
        case ProcessStatus::STOP:
            throw detail::StopTraversal();
            break;
        default:
            LOG_F(FIXME, "Unimplemented status value %d", (int)status);
            throw logic_error("Unimplemented status value");
            break;
        }

    } //foreach entry
loopExit:

    LOG_F(FIXME, "not yet implemented");
} // traverseDir()

// === GlobstariDisk =====================================================

std::vector<GlobstariBase::Entry>
GlobstariDisk::readDir(const Path& dirName)
{
    LOG_F(FIXME, "not yet implemented");
    return {};
}

bool
GlobstariDisk::readFile(const Path& dirPath, const Path& name, Bytes& contents)
{
    LOG_F(FIXME, "not yet implemented");
    return false;
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
