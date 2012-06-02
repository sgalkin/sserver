#ifndef SSERVER_EXCEPTION_H_INCLUDED
#define SSERVER_EXCEPTION_H_INCLUDED

#include <sstream>
#include <stdexcept>
#include <cstring>

#define THROW(message)                          \
    do {                                        \
        std::stringstream s;                    \
        s << message;                           \
        throw std::runtime_error(s.str());      \
    } while(false)                              \

#define REQUIRE(cond, message)                  \
    do {                                        \
        if(!(cond)) THROW(message);             \
    } while(false)                              \

// GCC defines _GNU_SOURCE by default
#define CHECK_CALL(callee, message)                                     \
    do {                                                                \
        if((callee) == -1) {                                            \
            char dummy;                                                 \
            THROW(message << strerror_r(errno, &dummy, sizeof(dummy))); \
        }                                                               \
    } while(false)                                                      \

#define CHECK_CALL_ERROR(callee, message, error)    \
    do {                                            \
        int r = (callee);                           \
        if(r != 0) THROW(message << (error(r)));    \
    } while(false)                                  \


#endif // SSERVER_EXCEPTION_H_INCLUDED
