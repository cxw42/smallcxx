/// @file smallcxx/common.hpp
/// @brief Common definitions for smallcxx
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#ifndef SMALLCXX_COMMON_HPP_
#define SMALLCXX_COMMON_HPP_

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

/// @name Bit-field manipulation macros
/// @{

/// Test if flag \p F is set in value \p V
#define HAS_FLAG(V, F) ( ((V) & (F)) == (F) )

/// Test if any flags \p FS are set in value \p V
#define HAS_ANY_FLAG(V, FS) ( ((V) & (FS)) != 0 )

/// OR together two enum values that represent fields.
#define OR_BITWISE(a, b) ( (decltype(a))( ((int)(a)) | ((int)(b)) ) )

/// @}

/// @name Compiler-related macros
/// @{

/// Mark an unreferenced parameter
#define UNREFERENCED_PARAMETER(p) (void)(p)
/// @}

#endif // SMALLCXX_COMMON_HPP_
