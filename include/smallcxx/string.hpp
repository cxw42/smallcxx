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
    std::ostringstream ss_;

    /// Whether we have data, even if that data is empty.
    /// Set to true when operator<<() is called.
    bool hasData_ = false;

public:

    template<class T>
    StringFormatter&
    operator <<(T&& rhs)
    {
        hasData_ = true;
        ss_ << rhs;
        return *this;
    }

    bool hasData() const { return hasData_; }

    operator std::string()
    {
        return ss_.str();
    }

    std::string
    str()
    {
        return ss_.str();
    }

    const char *
    c_str() const
    {
        // gcc accepts ss_.str().c_str() but clang does not.
        static std::string backing_storage;
        backing_storage = ss_.str();
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

namespace smallcxx
{

/// Trim leading and trailing whitespace in a string
/// @param[in] s - the string
/// @return a copy with any leading and trailing whitespace removed
std::string trim(const std::string& s);

} // namespace smallcxx
#endif // SMALLCXX_STRING_HPP_
