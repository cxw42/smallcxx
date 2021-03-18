/// @file common.hpp
/// @brief Common definitions for smallcxx
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <stdexcept>
#include <string.h>

#define ARRAY_SIZE(x) \
    (sizeof(x) / sizeof(x[0]))

/// Throw a message indicating an assertion failed, iff \p cond is false.
#define throw_assert(cond) \
    do { \
        const bool _ok = (cond); \
        if(!_ok) { \
            throw std::runtime_error(STR_OF << __FILE__ << ':' << __LINE__ \
                    << ": failure in assertion " << #cond); \
        } \
    } while(0)

#endif // COMMON_HPP_
