#ifndef SSERVER_TRAITS_H_INCLUDED
#define SSERVER_TRAITS_H_INCLUDED

#include <vector>
#include <string>

template<typename T, size_t S = sizeof(typename T::value_type)> 
struct IsByteArray;

template<typename U> 
struct IsByteArray<std::vector<U>, sizeof(char)> { 
    typedef void type; 
};

template<>
struct IsByteArray<std::string> {
    typedef void type; 
};

#endif // SSERVER_TRAITS_H_INCLUDED
