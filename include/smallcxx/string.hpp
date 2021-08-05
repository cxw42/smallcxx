/// @file smallcxx/string.hpp
/// @brief String-related definitions for smallcxx
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#ifndef SMALLCXX_STRING_HPP_
#define SMALLCXX_STRING_HPP_

#include <string>
#include <sstream>

// === String manipulation ===============================================

/// A wrapper around an ostringstream so you can use it inline.
/// @warning Any particular instance should be used only from
/// a single thread.
class StringFormatter
{
    std::ostringstream ss;
public:

    template<class T>
    StringFormatter&
    operator <<(T&& rhs)
    {
        ss << rhs;
        return *this;
    }

    operator std::string()
    {
        return ss.str();
    }

    std::string
    str()
    {
        return ss.str();
    }

    const char *
    c_str() const
    {
        // gcc accepts ss.str().c_str() but clang does not.
        static std::string backing_storage;
        backing_storage = ss.str();
        return backing_storage.c_str();
    }
};

/// Sugar for creating strings using iostream syntax.
/// E.g., `string foo = STR_OF << "answer=" << answer;`
#define STR_OF (StringFormatter())

#ifdef SMALLCXX_USE_CHOMP
/// Remove trailing `\n` from @p str.
/// Only declared if SMALLCXX_USE_CHOMP is `#define`d, since you might have
/// your own `chomp()` you want to use.
void chomp(char *str);
#endif // SMALLCXX_USE_CHOMP

#endif // SMALLCXX_STRING_HPP_
