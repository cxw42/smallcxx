/// @file src/globstari-matcher.cpp
/// @brief globstar + ignore routines --- Matcher implementation.
/// @details Part of smallcxx
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White
/// @copyright Uses some code from editorconfig-core-c, licensed BSD.
///     See below for the editorconfig-core-c copyright statement.
/// @todo permit matching only directories (trailing slash on globs)

#define SMALLCXX_LOG_DOMAIN "glob"

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
using smallcxx::glob::PathCheckResult;

namespace smallcxx
{
namespace glob
{

// from globstari.cpp
extern const string ec_special_chars;

// --- Matcher: Initializing ---------------------------------------------

Matcher::Matcher(const std::shared_ptr<Matcher>& delegate)
    : delegate_(delegate)
{
}

Matcher::Matcher(const std::initializer_list<smallcxx::glob::Path>& globs,
                 const smallcxx::glob::Path path,
                 const std::shared_ptr<Matcher>& delegate
                )
    : delegate_(delegate)
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
    while((pos = pathNoSlash.find_first_of(ec_special_chars,
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
    return (check(path) == PathCheckResult::Included);
} // Matcher::contains()

PathCheckResult
Matcher::check(const smallcxx::glob::Path& path) const
{
    if(!ready()) {
        throw logic_error("Matcher: Call to check() or contains() when not ready --- call finalize() after adding globsets");
    }

    if(path.empty()) {  // no matcher contains an empty path
        return PathCheckResult::Unknown;
    }

    if(*path.cbegin() != '/') {
        throw domain_error(
            "Matcher::contains: path must be absolute (start with /)");
    }

    // Check the globsets from back to front because later entries override
    // earlier entries.
    for(auto it = globsets_.crbegin(); it != globsets_.crend(); ++it) {
        if(it->globSet.contains(path)) {
            return (it->polarity == Polarity::Include) ?
                   PathCheckResult::Included : PathCheckResult::Excluded;
        }
    }

    return delegate_ ? delegate_->check(path) : PathCheckResult::Unknown;
} // Matcher::check()

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
