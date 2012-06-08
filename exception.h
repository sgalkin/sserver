#ifndef SSERVER_EXCEPTION_H_INCLUDED
#define SSERVER_EXCEPTION_H_INCLUDED

#include <sstream>
#include <stdexcept>
#include <cstring>
#include <errno.h>

#define THROW(message) do {                         \
        std::stringstream s_THROW;                  \
        s_THROW << message;                         \
        throw std::runtime_error(s_THROW.str());    \
    } while(false)                                  \

#define REQUIRE(cond, message) do {         \
        if(!(cond)) THROW(message);         \
    } while(false)                          \

// GCC defines _GNU_SOURCE by default
#define CHECK_CALL(callee, message) do {                                \
        if((callee) == -1) {                                            \
            char dummy_CHECK_CALL;                                      \
            THROW(message << ": "                                       \
                  << strerror_r(errno, &dummy_CHECK_CALL, sizeof(dummy_CHECK_CALL))); \
        }                                                               \
    } while(false)                                                      \

#define CHECK_CALL_ERROR(callee, message, error) do {                   \
        int r_CHECK_CALL_ERROR = (callee);                              \
        if(r_CHECK_CALL_ERROR != 0) {                                   \
            THROW(message << ": " << (error(r_CHECK_CALL_ERROR)));      \
        }                                                               \
    } while(false)                                                      \


#endif // SSERVER_EXCEPTION_H_INCLUDED
