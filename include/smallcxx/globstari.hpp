/// @file include/smallcxx/globstari.hpp
/// @brief globstar + ignore routines --- part of smallcxx
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White
/// SPDX-License-Identifier: BSD-3-Clause

#ifndef SMALLCXX_GLOBSTARI_HPP_
#define SMALLCXX_GLOBSTARI_HPP_

#include <cstdint>
#include <string>
#include <vector>

namespace smallcxx
{

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

    /// A path or filename.  A convenience typedef, to permit future changes.
    using Path = std::string;

    /// A chunk of data.  A convenience typedef, to permit future changes.
    using Bytes = std::vector<uint8_t>;

    /// Abstract type of an entry.
    /// @todo symlinks?
    enum class EntryType {
        File,   ///< Don't recurse into this
        Dir,    ///< Something we might recurse into
    };

    /// A single entry in a directory
    struct Entry {
        EntryType ty;   ///< what type this represents
        Path dirPath;   ///< path of the directory this Entry is in
        Path name;      ///< name of this Entry within @c dirPath

        Entry(EntryType newTy, const Path& newDirPath, const Path& newName)
            : ty(newTy), dirPath(newDirPath), name(newName) {}
    };

    /// Status values processEntry() can return.
    enum class ProcessStatus {
        CONTINUE,   ///< keep going
        POP,        ///< don't process any more entries in this directory
        STOP,       ///< don't process any more entries at all
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
    virtual std::vector<Entry> readDir(const Path& dirName) = 0;

    /// Returns the full content of a file, if the file exists.
    /// @param[in]  dirPath - the path to the directory
    /// @param[in]  name - the path to the file within @p dirPath
    /// @param[out] contents - the contents
    /// @return True if the file could be read; false otherwise.
    virtual bool readFile(const Path& dirPath, const Path& name,
                          Bytes& contents) = 0;

    /// Process an entry!  This can do whatever you want.
    /// The entry can be a directory or a file.
    /// @param[in]  entry - the entry
    /// @return A ProcessStatus value
    virtual ProcessStatus processEntry(const Entry& entry) = 0;

    /// @}

    /// @name Methods to call on an instance
    /// @{

    /// Find files in @p basePath matching @p globs.
    /// @param[in]  basePath - where to start.  This does not have to be
    ///     a directory on disk.
    /// @param[in]  globs - EditorConfig-style globs indicating the files
    ///     to find.  These are with respect to @p basePath.
    /// @param[in]  maxDepth - maximum recursion depth.  -1 for unlimited.
    void traverse(const Path& basePath, const std::vector<Path>& globs,
                  ssize_t maxDepth = -1);

    /// @}

private:

    /// Recursive traversal worker
    /// @todo Move to a pimpl?
    /// @param[in]  dirName - directory being traversed
    /// @param[in]  globs - globs of interest (TODO change to a regex)
    /// @param[in]  maxDepth - @see traverse()
    /// @param[in]  depth - current depth
    void traverseDir(const Path& dirName, const std::vector<Path>& globs,
                     ssize_t maxDepth, ssize_t depth);
}; // class GlobstariBase

/// Globstari for disk files (abstract base class).
/// This implements GlobstariBase::readDir() and GlobstariBase::readFile()
/// for files on disk.
class GlobstariDisk: public GlobstariBase
{
public:
    std::vector<Entry> readDir(const Path& dirName) override;
    bool readFile(const Path& dirPath, const Path& name, Bytes& contents) override;
}; // class GlobstariDisk

} // namespace smallcxx
#endif // SMALLCXX_GLOBSTARI_HPP_
