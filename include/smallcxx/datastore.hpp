/// @file datastore.hpp
/// @brief Header for centralized datastore
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White
///

#ifndef DATASTORE_HPP_
#define DATASTORE_HPP_

#include <smallcxx/common.hpp>

/// Basic Myers singleton
/// @tparam T   Class to instantiate.  Must be default-constructible.
template<class T>
struct Singleton {
public:
    static T *get()
    {
        static T *instance = new T();
        return instance;
    }
};

// TODO InterfaceProvider that inherits from a TypeList of template params


#endif // DATASTORE_HPP_
