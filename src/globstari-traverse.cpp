/// @file src/globstari-traverse.cpp
/// @brief globstar + ignore routines --- tree-traversal routines
/// @details Part of smallcxx
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White
/// @todo permit matching only directories (trailing slash on globs)

#define SMALLCXX_LOG_DOMAIN "glob"

#include <dirent.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <system_error>

#include "smallcxx/globstari.hpp"
#include "smallcxx/logging.hpp"
#include "smallcxx/string.hpp"

using namespace std;
using smallcxx::glob::Matcher;
using smallcxx::glob::PathCheckResult;

namespace smallcxx
{

// === Internal classes ==================================================

/// Human-readable PathCheckResultNames.  All the same width to make the
/// logs easier to read.
static const char *PathCheckResultNames[3] = {
    "included",
    "excluded",
    "unknown ",
};

/// Control exception used to stop traversal early
struct StopTraversal {};

#if 0
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
#endif

// === Breadth-first seach core ==========================================

using MatcherPtr = std::shared_ptr<Matcher>;

/// An entry and corresponding ignores
/// @invariant WorkItem::ignores is not NULL (but may be empty).
struct WorkItem {
    Entry entry;
    MatcherPtr ignores;

    WorkItem(const Entry& newEntry)
        : WorkItem(newEntry, make_shared<Matcher>())
    {}
    WorkItem(const Entry& newEntry, MatcherPtr newIgnores)
        : entry(newEntry), ignores(newIgnores)
    {
        if(!ignores) {
            throw logic_error("WorkItem: ignores must not be null");
        }
    }

    WorkItem(const WorkItem&) = default;
    ~WorkItem() = default;

}; // class WorkItem

/// Implementation of globstari()
class Traverser
{
    /// The hierarchy to search
    IFileTree& fileTree_;

    /// Work queue for breadth-first search
    std::deque<WorkItem> items_;

    /// What we are looking for
    smallcxx::glob::Matcher needleMatcher_;

    /// How low can you go?
    ssize_t maxDepth_;

    /// What to do with entries
    IProcessEntry& processEntry_;

    glob::PathSet seen_;    ///< which paths we have seen so far

    bool traversed_;        ///< have we already been run?

public:
    Traverser(IFileTree& fileTree, IProcessEntry& processEntry,
              const smallcxx::glob::Path& basePath,
              const std::vector<smallcxx::glob::Path>& needle,
              ssize_t maxDepth
             )
        : fileTree_(fileTree), maxDepth_(maxDepth), processEntry_(processEntry),
          traversed_(false)
    {
        smallcxx::glob::Path rootPath = fileTree_.canonicalize(basePath);
        needleMatcher_.addGlobs(needle,
                                rootPath.back() == '/' ? rootPath : rootPath + "/");
        needleMatcher_.finalize();

        // Prime the pump.  Note: the ignores start out empty, so this
        // first entry will not be ignored.
        items_.emplace_back(Entry{EntryType::Dir, rootPath, 0});
    }

    /// Run the traversal.
    /// @note Only one traversal per instantiation of Traverser!
    void run();

private:
    /// Do the work
    void worker();

    /// Prepare to descend into a directory
    void loadDir(const Entry& entry, MatcherPtr parentIgnores);

    /// Load the contents of ignore files
    MatcherPtr loadIgnoreFiles(const smallcxx::glob::Path& relativeTo,
                               std::vector<smallcxx::glob::Path> loadFrom,
                               MatcherPtr parentIgnores);

    /// Read ignore file @p contents, which lives in @p relativeTo, and add its
    /// contents to @p retval.
    void
    parseContentsInto(const Bytes& contents, Matcher& retval,
                      const smallcxx::glob::Path& relativeTo);
}; // class Traverser

void
Traverser::run()
{
    if(traversed_) {
        throw logic_error("Cannot call Traverser::run() more than once");
    }

    traversed_ = true;

    try {
        worker();
    } catch(StopTraversal&) {
        // Nothing to do
    } // other exceptions propagate out
} // Traverser::run()

void
Traverser::worker()
{
    while(!items_.empty()) {

        // Not auto& --- copy the item so we can pop_front() right away
        const auto item = items_.front();
        items_.pop_front();

        if(seen_.count(item.entry.canonPath)) {
            LOG_F(TRACE, "already-seen %s --- skipping",
                  item.entry.canonPath.c_str());
            continue;
        }

        // TODO make sure this is in the right place
        seen_.insert(item.entry.canonPath);

        if((maxDepth_ > 0) && (item.entry.depth > maxDepth_)) {
            LOG_F(TRACE, "Skipping %s --- maxDepth exceeded",
                  item.entry.canonPath.c_str());
            continue;
        }

        // Check against the ignores we already have
        if(item.ignores->contains(item.entry.canonPath)) {
            LOG_F(TRACE, "ignored %s --- skipping",
                  item.entry.canonPath.c_str());
            continue;
        }

        // Is it a hit?
        const auto match = needleMatcher_.check(item.entry.canonPath);

        LOG_F(TRACE, "pathcheck:%s for [%s]", PathCheckResultNames[(int)match],
              item.entry.canonPath.c_str());

        // Decide what to do
        auto clientInstruction = IProcessEntry::Status::Continue;

        if(match == PathCheckResult::Excluded) {
            // Excluded => nothing more to do.  Simple!
            continue;

        } else if(match == PathCheckResult::Included) {
            // Included => give it to the client.  Also simple!
            clientInstruction = processEntry_(item.entry);

        } else if(item.entry.ty == EntryType::Dir) {
            // But directories not specifically included may contain
            // files that are themselves included.  Therefore,
            // descend into directories if match == Unknown.
            loadDir(item.entry, item.ignores);
            continue;
        }

        // Do what the client asked us to
        switch(clientInstruction) {
        case IProcessEntry::Status::Continue:
            if(item.entry.ty == EntryType::Dir) {
                loadDir(item.entry, item.ignores);
            }
            break;

        case IProcessEntry::Status::Skip:
            // nothing to do
            break;

#if 0
        // TODO implement this?
        case IProcessEntry::Status::Pop:
            // TODO pop the deque until `dir` changes
            break;
#endif

        case IProcessEntry::Status::Stop:
            throw StopTraversal();
            break;

        default:
            throw logic_error(STR_OF << "Unimplemented status value "
                              << (int)clientInstruction);
            break;
        } //switch(clientInstruction)

    } // while entries remain
} // Traverser::worker()

void
Traverser::loadDir(const Entry& entry, MatcherPtr parentIgnores)
{
    // Load the new ignores
    auto ignoresToLoad = fileTree_.ignoresForDir(entry.canonPath);
    auto ignores = loadIgnoreFiles(entry.canonPath + "/",
                                   ignoresToLoad, parentIgnores);

    // Load the new entries
    auto newEntries = fileTree_.readDir(entry.canonPath);
    for(auto& newEntry : newEntries) {
        newEntry.depth = entry.depth + 1;
        items_.emplace_back(newEntry, ignores);
    }
} // Traverser::loadDir()

/// @todo Document and verify which paths have to end with a /
MatcherPtr
Traverser::loadIgnoreFiles(const smallcxx::glob::Path& relativeTo,
                           std::vector<smallcxx::glob::Path> loadFrom,
                           MatcherPtr parentIgnores)
{
    MatcherPtr retval(new Matcher(parentIgnores));

    for(const auto& toLoad : loadFrom) {
        bool ok = false;
        glob::Path pathToTry, canonPath;
        Bytes contents;

        if(!toLoad.empty() && toLoad.front() == '/') {  // absolute
            pathToTry = canonPath = toLoad;
        } else {
            pathToTry = relativeTo;
            pathToTry += '/';
            pathToTry += toLoad;
            canonPath = fileTree_.canonicalize(pathToTry);
        }

        if(!canonPath.empty()) {
            try {
                contents = fileTree_.readFile(canonPath);
                ok = true;
            } catch(...) {
                ok = false;
            }
        }

        if(!ok) {
            LOG_F(LOG, "skipping non-existent or unreadable ignore-file candidate %s",
                  pathToTry.c_str());
            continue;
        }

        parseContentsInto(contents, *retval, relativeTo);
    }
    retval->finalize();

    return retval;
}

/// @todo make this part of the public API
void
Traverser::parseContentsInto(const Bytes& contents,
                             Matcher& retval,
                             const smallcxx::glob::Path& relativeTo)
{
    istringstream ss(contents);
    string s;
    while(getline(ss, s)) {
        auto pattern = trim(s);   // no leading/trailing ws
        if(pattern.empty() || pattern[0] == '#') {
            continue;
        }

        // Check for non-escaped #
        for(size_t idx = 1; idx < pattern.size(); ++idx) {
            if( (pattern[idx] == '#') && (pattern[idx - 1] != '\\') ) {
                pattern = trim(pattern.substr(0, idx));
                break;
            }
        }

        retval.addGlob(pattern, relativeTo);
    }
}

// === IFileTree and globstari() =========================================

std::vector<smallcxx::glob::Path>
IFileTree::ignoresForDir(const smallcxx::glob::Path& dirName)
{
    return { ".eignore" };
}

void
globstari(IFileTree& fileTree,
          IProcessEntry& processEntry,
          const smallcxx::glob::Path& basePath,
          const std::vector<smallcxx::glob::Path>& needle,
          ssize_t maxDepth)
{
    Traverser t(fileTree, processEntry, basePath, needle, maxDepth);
    t.run();
}

// === DiskFileTree ======================================================

/// @todo PORTABILITY: handle readdir() that doesn't set d_type
std::vector<Entry>
DiskFileTree::readDir(const smallcxx::glob::Path& dirName)
{
    std::unique_ptr<DIR, void(*)(DIR *)> dirp(
        opendir(dirName.c_str()),
    [](DIR * d) {
        closedir(d);
    }
    );

    if(!dirp) {
        throw system_error(errno, std::generic_category(),
                           STR_OF << "Could not open dir" << dirName);
    }

    std::vector<Entry> retval;

    struct dirent *ent;
    while((ent = readdir(dirp.get())) != NULL) {
        const auto canonPath = dirName + "/" + ent->d_name;

        EntryType ty;
        if(ent->d_type == DT_REG) {
            ty = EntryType::File;

        } else if(ent->d_type == DT_DIR) {
            ty = EntryType::Dir;
            if((strcmp(ent->d_name, ".") == 0) ||
                    (strcmp(ent->d_name, "..") == 0)) {
                continue;
            }

        } else {
            LOG_F(TRACE, "Skipping [%s] of type %c",
                  canonPath.c_str(), ent->d_type);
            continue;
        }

        LOG_F(TRACE, "Found %s [%s]",
              (ty == EntryType::File ? "file" : "dir"),
              canonPath.c_str());
        retval.emplace_back(ty, canonPath);
    } // foreach dir entry

    return retval;
}

/// @todo make this more efficient (fewer copies)
Bytes
DiskFileTree::readFile(const smallcxx::glob::Path& path)
{
    ifstream in(path);
    ostringstream os;
    os << in.rdbuf();
    return os.str();
}

/// @todo document symlink behaviour.  realpath(3) removes them,
///     so other implementations should do so also.  (Right?)
smallcxx::glob::Path
DiskFileTree::canonicalize(const smallcxx::glob::Path& path) const
{
    unique_ptr<char, void(*)(char *)> resolved(
        realpath(path.c_str(), nullptr),
    [](char *p) {
        free((void *)p);
    }
    );

    if(resolved) {
        return smallcxx::glob::Path(resolved.get());
    }

    if(errno == ENOENT) {
        return "";
    }

    throw system_error(errno, std::generic_category(),
                       STR_OF << "Could not resolve path " << path);
}

} // namespace smallcxx
