/// @file include/smallcxx/globstari.hpp
/// @brief globstar + ignore routines --- part of smallcxx
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White
/// SPDX-License-Identifier: BSD-3-Clause

#ifndef SMALLCXX_GLOBSTARI_HPP_
#define SMALLCXX_GLOBSTARI_HPP_

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <deque>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace smallcxx
{

// === Testing paths against globs =======================================

/// Glob-testing routines.  Used internally by GlobstariBase, but exposed here
/// for testability and because they might be useful in other contexts.
namespace glob
{

// --- PCRE2-related types -----------------------------------------------

using MatchDataPtr =
    std::unique_ptr<pcre2_match_data, void(*)(pcre2_match_data *)>;

using RePtr = std::unique_ptr<pcre2_code, void(*)(pcre2_code *)>;

// --- GlobSet -----------------------------------------------------------

/// A path or filename.  A convenience typedef, to permit future changes.
using Path = std::string;

/// Element of a numerical range
using Int = intmax_t;

/// Numerical range
using IntPair = std::pair<Int, Int>;

/// List of numerical ranges
using RangePairs = std::vector<IntPair>;

/// Polarity of globs: include or exclude
enum class Polarity {
    Include,   ///< includes (don't start with `!`)
    Exclude,   ///< excludes (start with `!`)
};

/// Set of globs
using PathSet = std::unordered_set<smallcxx::glob::Path>;

/// A set of globs.
///
/// - Each glob must match against the entire string.  E.g., `*.txt` will match
///     `foo.txt` but not `foo/bar.txt`.
/// - Glob checks do not treat dot files specially.  Therefore,
///     `*foo` will match `foo` and `.foo`.
/// - No globset matches an empty string.
class GlobSet
{
    PathSet globs_;             ///< individual globs

    /// regex matching everything in globs_
    RePtr compiled_;

    /// Numeric ranges for capture groups listed in compiled_
    RangePairs ranges_;

public:

    GlobSet();
    GlobSet(const GlobSet& other);

    /// Add a single glob to the set.
    void addGlob(const smallcxx::glob::Path& glob);

    /// Add multiple globs.
    /// Templated so you can use vectors or initializer_lists.
    template<class T>
    void
    addGlobs(const T& globs)
    {
        for(const auto& p : globs) {
            addGlob(p);
        }
    }

    /// Fill in compiled_ from globs_.
    /// @throws std::runtime_error if the regex cannot be constructed.
    /// @note Calling finalize() without first adding any globs is not
    ///     an error!  It will give you a GlobSet that matches nothing.
    void finalize();

    /// Whether finalize() has been called
    bool
    finalized() const
    {
        return (bool)compiled_;
    }

    /// Returns true if the GlobSet contains @p path.
    /// @throws logic_error if finalize() has not been called.
    /// @param[in]  path - the path to check.  Must be either the empty
    ///     string (in which case it doesn't match) or an absolute path.
    /// @returns True if @p path is in this GlobSet; false otherwise.
    bool contains(const smallcxx::glob::Path& path) const;

}; // class GlobSet

// --- Matcher -----------------------------------------------------------

/// The state of a path w.r.t. a Matcher
enum class PathCheckResult {
    Included,   ///< Listed in a Polarity::Include set
    Excluded,   ///< Listed in a Polarity::Exclude set
    Unknown,    ///< Not listed
};

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
/// will have, in order, a Polarity::Include set matching `*.{bak,swp}`;
/// a Polarity::Exclude set matching `*.foo`; and a Polarity::Include set
/// matching `*.bar`.
class Matcher
{
private:

    /// A GlobSet and its polarity
    struct SetAndPolarity {
        GlobSet globSet;
        Polarity polarity;

        SetAndPolarity(Polarity pol): globSet(), polarity(pol) {}
        SetAndPolarity(const SetAndPolarity& other) = default;
    };

    /// The globs we match (or don't).
    /// Invariant:
    /// - until finalize() is called, all globsets except the last one have
    ///   been finalized.
    /// - once finalize() completes, all globsets have been finalized.
    std::vector<SetAndPolarity> globsets_;

public:

    Matcher() = default;
    ~Matcher() = default;

    /// Construct and finalize a matcher with multiple calls to
    /// addGlob(Path,Path).
    /// @param[in]  globs - the globs to add
    /// @param[in]  path - where each of the globs should be anchored.  Must be
    ///     nonempty and end with a `/`.
    Matcher(const std::initializer_list<smallcxx::glob::Path>& globs,
            const smallcxx::glob::Path path);

    /// Add a single glob to the matcher
    void addGlob(const smallcxx::glob::Path& glob);

    /// Add a single glob, relative to a path, to the matcher
    /// @param[in]  glob - the glob
    /// @param[in]  path - where the glob should be anchored.  Must be
    ///     nonempty and end with a `/`.
    ///
    /// E.g., if @p path is `/foo` and @p glob is `*.txt`, only
    /// `/foo/*.txt` will match.  However, if @p glob is `**/*.txt`,
    /// `/foo/bar/*.txt` will also match.
    void addGlob(const smallcxx::glob::Path& glob,
                 const smallcxx::glob::Path& path);

    /// Add multiple globs to the matcher.
    /// Templated so you can use vectors or initializer_lists.
    template<class T>
    void
    addGlobs(const T& globs)
    {
        for(const auto& g : globs) {
            addGlob(g);
        }
    }

    /// Add multiple globs to the matcher.
    /// Templated so you can use vectors or initializer_lists.
    template<class T>
    void
    addGlobs(const T& globs, const smallcxx::glob::Path& path)
    {
        for(const auto& g : globs) {
            addGlob(g, path);
        }
    }

    /// Call this once all the globs have been added
    void
    finalize();

    /// Whether you can run contains()
    bool
    ready() const;

    /// Check whether the Matcher contains @p path.
    /// @param[in]  path - the path to check.  Must be either the empty
    ///     string (in which case it doesn't match) or an absolute path.
    /// @returns True if @p path is in this GlobSet; false otherwise.
    /// @throws logic_error if not ready().
    /// @returns
    /// - true if @p path is included, or
    /// - false if:
    ///   - @p path is excluded,
    ///   - @p path does not match any globset in the Matcher, regardless of
    ///     polarity, or
    ///   - there are no globsets in the Matcher.
    bool
    contains(const smallcxx::glob::Path& path) const;

    /// Check whether @p path is included, excluded, or not in this Matcher.
    /// @param[in]  path - the path to check.  Must be either the empty
    ///     string (in which case the result is Unknown) or an absolute path.
    /// @throws logic_error if not ready().
    /// @returns
    /// - PathCheckResult::Included if @p path is included;
    /// - PathCheckResult::Excluded if @p path is excluded,
    /// - PathCheckResult::Unknown if:
    ///   - @p path does not match any globset in the Matcher, regardless of
    ///     polarity, or
    ///   - there are no globsets in the Matcher.
    PathCheckResult check(const smallcxx::glob::Path& path) const;

}; // class Matcher

} // namespace glob

// === Searching through directories =====================================

/// SAX-style directory searcher: base class.
/// Subclass this and implement the virtual functions to do your own traversing.
/// All globs follow the [EditorConfig](https://editorconfig.org) format.
/// @note Path entries are separated by `/` (forward slash) on all platforms!
///
/// "Globstari" = supports _glob_, glob_star_, and _i_gnores.
///
/// @todo
/// - Symlinks?
/// - If so, symlink-loop checking?
class GlobstariBase
{
public:

    // --- Types ---

    /// A chunk of data.  A convenience typedef, to permit future changes.
    using Bytes = std::vector<uint8_t>;

    /// Abstract type of an entry.
    /// @todo symlinks?
    enum class EntryType {
        File,       ///< Don't recurse into this
        Dir,        ///< Something we might recurse into

        /// The starting directory of the search
        /// @todo See if there is a cleaner way to handle this.
        StartDir,
    };

    /// A single entry in a directory
    struct Entry {
        EntryType ty;   ///< what type this represents

#if 0
        /// Path of the directory this Entry is in
        smallcxx::glob::Path dirPath;

        /// Name of this Entry within @c dirPath
        smallcxx::glob::Path name;
#endif

        /// Canonical path of this Entry
        smallcxx::glob::Path canonPath;

        int depth;  ///< level 0 is the root dir

        Entry(EntryType newTy, const smallcxx::glob::Path& newCanonPath,
              int newDepth)
            : ty(newTy), canonPath(newCanonPath), depth(newDepth) {}

        Entry(const Entry& other) = default;
        Entry(Entry&& other) = default;

    }; // struct Entry

    /// Status values processEntry() can return.
    enum class ProcessStatus {
        Continue,   ///< keep going
#if 0
        // TODO implement this?
        Pop,        ///< don't process any more entries in this directory
#endif
        Stop,       ///< don't process any more entries at all

        /// For a directory, don't descend into the dir.
        /// For a file, treated the same as Continue.
        Skip,

    };

    // --- Routines ---

    GlobstariBase() = default;
    virtual ~GlobstariBase() = default;

    /// @name Virtual functions to implement
    /// @{

    /// Returns a list of the names of the entries in @p path.
    /// You do not have to take ignores into account.
    /// @param[in]  dirName - the path to the directory itself
    /// @return The entries, if any.
    virtual std::vector<Entry> readDir(const smallcxx::glob::Path& dirName) = 0;

    /// Returns the full content of a file, if the file exists.
    /// @param[in]  dirPath - the path to the directory
    /// @param[in]  name - the path to the file within @p dirPath
    /// @param[out] contents - the contents
    /// @return True if the file could be read; false otherwise.
    virtual bool readFile(const smallcxx::glob::Path& dirPath,
                          const smallcxx::glob::Path& name,
                          Bytes& contents) = 0;

    /// Process an entry!  This can do whatever you want.
    /// The entry can be a directory or a file.
    /// @param[in]  entry - the entry
    /// @return A ProcessStatus value
    virtual ProcessStatus processEntry(const Entry& entry) = 0;

    /// Canonicalize a path.
    /// @param[in]  path - the path to canonicalize
    /// @return The canonicalized path (absolute, no . or .., `/` separators)
    virtual smallcxx::glob::Path canonicalize(const smallcxx::glob::Path& path) = 0;

    /// @}

    /// @name Methods to call on an instance
    /// @{

    /// Find files in @p basePath matching @p needle.
    /// @param[in]  basePath - where to start.  This does not have to be
    ///     a directory on disk.
    /// @param[in]  needle - EditorConfig-style globs indicating the files
    ///     to find.  These are with respect to @p basePath.
    /// @param[in]  maxDepth - maximum recursion depth.  -1 for unlimited.
    ///
    /// @note @parblock
    ///
    /// - Glob checks do not treat dot files specially.  Therefore,
    ///     `*foo` will match `foo` and `.foo`.
    /// - All glob checks are done against canonicalized paths.
    ///     Therefore, `**/*` will match everything.
    /// - traverse() will always call processEntry on the starting dir.
    ///
    /// @endparblock
    void traverse(const smallcxx::glob::Path& basePath,
                  const std::vector<smallcxx::glob::Path>& needle,
                  ssize_t maxDepth = -1);

    /// @}

private:

    /// Traversal worker
    /// @todo Move to a pimpl?
    /// @param[in]  entries - queue to process.  We use a deque as a queue for
    ///     implementation reasons.
    /// @param[in]  rootPath - where we started
    /// @param[in]  needleMatcher - the paths of interest
    /// @param[in]  maxDepth - @see traverse()
    void traverseWorker(
        std::deque<Entry>& entries, const smallcxx::glob::Path rootPath,
        const smallcxx::glob::Matcher& needleMatcher,
        ssize_t maxDepth);
}; // class GlobstariBase

/// Globstari for disk files (abstract base class).
/// This implements GlobstariBase::readDir() and GlobstariBase::readFile()
/// for files on disk.
class GlobstariDisk: public GlobstariBase
{
public:
    std::vector<Entry> readDir(const smallcxx::glob::Path& dirName) override;
    bool readFile(const smallcxx::glob::Path& dirPath,
                  const smallcxx::glob::Path& name, Bytes& contents) override;
    smallcxx::glob::Path canonicalize(const smallcxx::glob::Path& path) override;
}; // class GlobstariDisk


} // namespace smallcxx
#endif // SMALLCXX_GLOBSTARI_HPP_
