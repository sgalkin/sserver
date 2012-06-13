#ifndef SSERVER_STORAGE_H_INCLUDED
#define SSERVER_STORAGE_H_INCLUDED

#include <boost/scoped_ptr.hpp>

template<typename T>
struct Own {};

template<typename T>
struct Storage {
    typedef T base;
    typedef T* type; 
    static base* get(T* ptr) { return ptr; }
    static const base* get(const T* ptr) { return ptr; }
};

template<typename T>
struct Storage< Own<T> > { 
    typedef T base;
    typedef boost::scoped_ptr<T> type; 
    static base* get(type& ptr) { return ptr.get(); }
    static const base* get(const type& ptr) { return ptr.get(); }
};

#endif // SSERVER_STORAGE_H_INCLUDED
