/// @file t/testhelpers.hpp
/// @brief Utilities for testing
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021--2022 Christopher White

#ifndef TESTHELPERS_HPP_
#define TESTHELPERS_HPP_

#include <map>
#include <set>
#include <utility>
#include <vector>

#if BUILD_GLOBSTARI
#include "smallcxx/globstari.hpp"

/// Test if @p got matches @p expectedPaths.
void
compare_sequence(const std::set<smallcxx::glob::Path>& got,
                 const std::vector<smallcxx::glob::Path>& expectedPaths,
                 const char *func, const size_t line);

/// Save filenames traversed.
/// For testing globbing on disk.
class SaveEntries: public smallcxx::IProcessEntry
{
public:
    using EntryMap =
        std::map<smallcxx::glob::Path, std::shared_ptr<smallcxx::Entry> >;
    std::set<smallcxx::glob::Path> found;
    EntryMap foundEntries;
    std::set<smallcxx::glob::Path> ignoredPaths;
    EntryMap ignoredEntries;

    IProcessEntry::Status
    operator()(const std::shared_ptr<smallcxx::Entry>& entry) override;

    void
    ignored(const std::shared_ptr<smallcxx::Entry>& entry) override;

}; // SaveEntries

#endif // BUILD_GLOBSTARI

#endif // TESTHELPERS_HPP_
