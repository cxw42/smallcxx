/// @file include/smallcxx/globstari.hpp
/// @brief globstar + ignore routines --- part of smallcxx
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021--2022 Christopher White
/// SPDX-License-Identifier: BSD-3-Clause
///
/// The primary function in this file is globstari().

#ifndef SMALLCXX_GLOBSTARI_HPP_
#define SMALLCXX_GLOBSTARI_HPP_

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

// --- GlobSet -----------------------------------------------------------

/// A path or filename.  A convenience typedef, to permit future changes.
using Path = std::string;

/// Set of globs or paths
using PathSet = std::unordered_set<smallcxx::glob::Path>;

/// Polarity of globs: include or exclude
enum class Polarity {
    Include,   ///< includes (don't start with `!` in an ignore file)
    Exclude,   ///< excludes (start with `!` in an ignore file)
};

/// pimpl for GlobSet
class GlobSetImpl;

/// A set of globs.
///
/// - Each glob must match against the entire string.  E.g., `*.txt` will match
///     `foo.txt` but not `foo/bar.txt`.
/// - Glob checks do not treat dot files specially.  Therefore,
///     `*foo` will match `foo` and `.foo`.
/// - No globset matches an empty string.
///
/// All globs follow the [EditorConfig](https://editorconfig.org) format.
/// @note Path entries are separated by `/` (forward slash) on all platforms!
class GlobSet
{
    std::unique_ptr<GlobSetImpl> impl_;

public:

    GlobSet();
    GlobSet(const GlobSet& other);
    ~GlobSet();

    /// Add a single glob to the set.
    /// @note contains() cannot be used until you have called finalize().
    /// @throws std::runtime_error if finalized()
    void addGlob(const smallcxx::glob::Path& glob);

    /// Add multiple globs.
    /// Templated so you can use vectors or initializer_lists.
    /// @note contains() cannot be used until you have called finalize().
    /// @throws std::runtime_error if finalized()
    template<class T>
    void
    addGlobs(const T& globs)
    {
        for(const auto& p : globs) {
            addGlob(p);
        }
    }

    /// Prepare a GlobSet to support addGlob() calls.
    /// @throws std::runtime_error on error
    /// @note Calling finalize() without first adding any globs is not
    ///     an error!  It will give you a GlobSet that matches nothing.
    void finalize();

    /// Whether finalize() has been called
    bool finalized() const;

    /// Returns true if the GlobSet contains @p path.
    /// @throws logic_error if finalize() has not been called.
    /// @param[in]  path - the path to check.  Must be either the empty
    ///     string (in which case it doesn't match) or an absolute path.
    /// @return True if @p path is in this GlobSet; false otherwise.
    bool contains(const smallcxx::glob::Path& path) const;

}; // class GlobSet

// --- Matcher -----------------------------------------------------------

/// The state of a path w.r.t. a Matcher
enum class PathCheckResult {
    Included,   ///< Listed in a Polarity::Include set
    Excluded,   ///< Listed in a Polarity::Exclude set
    Unknown,    ///< Not listed in any set in the Matcher
};

/// A matcher holding any number of glob patterns, include or exclude.
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
///
/// All globs follow the [EditorConfig](https://editorconfig.org) format.
/// @note Path entries are separated by `/` (forward slash) on all platforms!
class Matcher
{
private:

    /// If non-null, a Matcher to delegate to if a path is neither included
    /// nor excluded by this Matcher.
    std::shared_ptr<Matcher> delegate_;

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

    /// Construct a matcher having a delegate.
    /// @param[in] delegate - The Matcher to delegate to for unknown results.
    ///     E.g., a parent ignore set.
    explicit Matcher(const std::shared_ptr<Matcher>& delegate);

    /// Construct and finalize a matcher with multiple calls to
    /// addGlob(Path,Path).
    /// @param[in]  globs - the globs to add
    /// @param[in]  path - where each of the globs should be anchored.  Must be
    ///     nonempty.  May end with `/` or not.
    /// @param[in]  delegate - The Matcher to delegate to for unknown results.
    ///     E.g., a parent ignore set.  Optional; default nullptr.
    Matcher(const std::initializer_list<smallcxx::glob::Path>& globs,
            const smallcxx::glob::Path path,
            const std::shared_ptr<Matcher>& delegate = nullptr
           );

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
    /// @return True if @p path is in this GlobSet; false otherwise.
    /// @throws logic_error if not ready().
    /// @return
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
    /// @return
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

/// A chunk of data.  A convenience typedef, to permit future changes.
using Bytes = std::string;

/// Abstract type of an entry.
/// @todo symlinks?  Special files?
enum class EntryType {
    File,       ///< Don't recurse into this
    Dir,        ///< Something we might recurse into
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

    /// Depth of an entry with respect to a search.  Level 0 is the
    /// dir where the search started.  Negative values have undefined
    /// meaning.
    int depth;

    Entry(EntryType newTy, const smallcxx::glob::Path& newCanonPath,
          int newDepth = -1)
        : ty(newTy), canonPath(newCanonPath), depth(newDepth) {}

    Entry(const Entry& other) = default;
    Entry(Entry&& other) = default;

    /// Virtual dtor so callers can use polymorphic Entry types.
    virtual ~Entry() = default;

}; // struct Entry

/// Access to a hierarchical tree of files (not necessarily on disk).
/// Implemented by users of GlobstariBase.
class IFileTree
{
public:

    /// Create an entry for the root dir itself, as opposed to its contents.
    /// This function exists so you can use Entry subclasses uniformly
    /// throughout a globstari() call.
    ///
    /// The default implementation returns a smallcxx::Entry instance.
    ///
    /// @param[in]  rootPath - where to start.  Canonicalized.
    /// @return the new Entry
    virtual std::shared_ptr<Entry> rootDir(
        const smallcxx::glob::Path& rootPath
    );

    /// Returns a list of the names of the entries in @p dirName.
    /// You do not have to take ignores into account.
    ///
    /// @note You *do* have to handle `.` and `..`!  Do not return those,
    ///     or any other entries you do not want considered during traversal.
    ///
    /// @param[in]  dirName - the path to the directory itself, canonicalized.
    /// @return The entries, if any.  You do not have to fill in Entry::depth.
    /// @throws std::system_error if @p dirName is unreadable.
    ///
    /// @todo Permit iteratively reading a dir?
    virtual std::vector< std::shared_ptr<Entry> > readDir(const
            smallcxx::glob::Path& dirName) = 0;

    /// Returns a list of ignore files to load, if they exist, for @p dirName.
    /// @param[in]  dirName - the canonical path to the directory
    /// @return A list of zero or more ignore paths, absolute or relative.-
    ///     - If absolute, that file is checked.
    ///     - If relative, that file is checked in @p dirName.
    ///     Therefore, you can return a fixed vector if you only have
    ///     relative-path ignore files.
    ///
    /// The default ignore is `.eignore` in the current directory.
    /// The `e` in `.eignore` is because it uses `E`ditorConfig-style globs.
    ///
    /// @param[in]  dirName - the path to the directory, canonicalized.
    virtual std::vector<smallcxx::glob::Path> ignoresForDir(
        const smallcxx::glob::Path& dirName);

    /// Returns the full content of a file, if the file exists.
    /// @param[in]  path - the path to the file, canonicalized
    /// @return The contents
    /// @throws Exception (any) if an error occurs
    virtual Bytes readFile(const smallcxx::glob::Path& path) = 0;

    /// Canonicalize a path.
    /// @param[in]  path - the path to canonicalize
    /// @return
    /// - If the path does not exist, ""
    /// - Otherwise, the canonicalized path (absolute, without `.` or `..`,
    ///     and with `/` separators)
    virtual smallcxx::glob::Path canonicalize(const smallcxx::glob::Path& path)
    const = 0;

};

/// What to do with an item when you find it.
class IProcessEntry
{
public:
    /// Status values operator()() can return.
    enum class Status {
        Continue,   ///< keep going
#if 0
        // TODO implement this?
        Pop,        ///< don't process any more entries in this directory
#endif
        Stop,       ///< don't process any more entries at all

        /// Skip directories (no effect for files).
        /// This is the return value from the subclass's processing,
        /// so Skip doesn't mean "don't process this".
        ///
        /// - For a directory, don't descend into the dir.
        /// - For a file, treated the same as Continue.
        Skip,

    };

    /// Process an entry!  This can do whatever you want.
    /// The entry can be a directory or a file.
    /// @param[in]  entry - the entry
    /// @return A ProcessStatus value
    virtual IProcessEntry::Status operator()(const std::shared_ptr<Entry>& entry) =
        0;
};

/// Find files, inside the hierarchy accessible through @p fileTree,
/// that are under @p basePath and match @p needle.
/// @param[in]  fileTree - Access to the hierarchy to search
/// @param[in]  processEntry - what to do with each entry found
/// @param[in]  basePath - where to start.
/// @param[in]  needle - EditorConfig-style globs indicating the files
///     to find.  These are with respect to @p basePath.
///     **Note** @p needle must not be empty.
/// @param[in]  maxDepth - maximum recursion depth.  -1 for unlimited.
///
/// Bear in mind:
/// - All globs follow the [EditorConfig](https://editorconfig.org) format.
/// - Path entries are separated by `/` (forward slash) on all platforms!
///
/// @note @parblock
///
/// - Glob checks do not treat dot files specially.  Therefore,
///     `*foo` will match `foo` and `.foo`.
/// - All glob checks are done against canonicalized paths.
///     Therefore, `**/*` will match everything.
/// - globstari() will always call @p processEntry on @p basePath
///
/// @endparblock
///
/// "Globstari" = supports _glob_, glob_star_, and _i_gnores.
///
/// @todo @parblock
/// - Symlinks?
/// - If so, symlink-loop checking?
/// @endparblock
void globstari(IFileTree& fileTree,
               IProcessEntry& processEntry,
               const smallcxx::glob::Path& basePath,
               const std::vector<smallcxx::glob::Path>& needle,
               ssize_t maxDepth = -1);

/// Access to files on disk.  For use with globstari().
/// Assumes the root directory of the filesystem is the
class DiskFileTree: public IFileTree
{
public:
    virtual ~DiskFileTree() = default;
    std::vector< std::shared_ptr<Entry> > readDir(const smallcxx::glob::Path&
            dirName) override;
    Bytes readFile(const smallcxx::glob::Path& path) override;
    smallcxx::glob::Path canonicalize(const smallcxx::glob::Path& path) const
    override;
}; // class GlobstariDisk


} // namespace smallcxx
#endif // SMALLCXX_GLOBSTARI_HPP_
