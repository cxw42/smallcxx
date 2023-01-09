/// @file t/testhelpers.cpp
/// @brief Utilities for testing
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021--2022 Christopher White

#include "smallcxx/test.hpp"

#include "testhelpers.hpp"

#if BUILD_GLOBSTARI

TEST_FILE

#include "smallcxx/globstari.hpp"

void
compare_sequence(const std::set<smallcxx::glob::Path>& got,
                 const std::vector<smallcxx::glob::Path>& expectedPaths,
                 const char *func, const size_t line)
{
    // INFO log level so it will appear by default --- otherwise we don't know
    // which test failed.
    LOG_F(INFO, "Checking %s():%zu", func, line);

    cmp_ok(got.size(), ==, expectedPaths.size());
    size_t idx = 0;
    for(auto it = got.cbegin();
            (it != got.cend()) && (idx < expectedPaths.size());
            ++it, ++idx) {
        const auto& got = *it;
        const auto& expected = expectedPaths[idx];
        const auto where = got.rfind(expected);

        LOG_F(SNOOP, "got [%s], expected [%s]", got.c_str(), expected.c_str());
        cmp_ok(where, !=, got.npos);
    }
} // compare_sequence()

smallcxx::IProcessEntry::Status
SaveEntries::operator()(const std::shared_ptr<smallcxx::Entry>& entry)
{
    LOG_F(TRACE, "Found %s", entry->canonPath.c_str());
    found.insert(entry->canonPath);
    foundEntries[entry->canonPath] = entry;
    return IProcessEntry::Status::Continue;
}

void
SaveEntries::ignored(const std::shared_ptr<smallcxx::Entry>& entry)
{
    LOG_F(TRACE, "Found %s", entry->canonPath.c_str());
    ignoredPaths.insert(entry->canonPath);
    ignoredEntries[entry->canonPath] = entry;
}
#endif // BUILD_GLOBSTARI
