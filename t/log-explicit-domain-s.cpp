/// @file log-explicit-domain-s.cpp
/// @brief Helper used by t/logging-t.sh
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2022 Christopher White

/// Log domain.  This is an explicit domain, so does not participate in '*:lev'.
#define SMALLCXX_LOG_DOMAIN "+fruit"

#include "smallcxx/logging.hpp"

using namespace std;

int
main()
{
    setVerbosityFromEnvironment("LOG_LEVELS");
    LOG_F(DEBUG, "avocado");
    return 0;
}
