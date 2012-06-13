#ifndef SSERVER_EXCEPTION_H_INCLUDED
#define SSERVER_EXCEPTION_H_INCLUDED

#include <sstream>
#include <string>
#include <stdexcept>
#include <cstring>
#include <errno.h>

// GCC defines _GNU_SOURCE by default
inline std::string string_error(int code) {
    char dummy;
    return strerror_r(code, &dummy, sizeof(dummy));
}

#define THROW(message) do {                         \
        std::stringstream s_THROW;                  \
        s_THROW << message;                         \
        throw std::runtime_error(s_THROW.str());    \
    } while(false)                                  \

#define REQUIRE(cond, message) do {         \
        if(!(cond)) THROW(message);         \
    } while(false)                          \

#define CHECK_CALL(callee, message)                                 \
    REQUIRE((callee) != -1, message << ": " << string_error(errno)) \

#define CHECK_CALL_ERROR(callee, message, error) do {                   \
        int r_CHECK_CALL_ERROR = (callee);                              \
        REQUIRE(r_CHECK_CALL_ERROR == 0,                                \
                message << ": " << (error(r_CHECK_CALL_ERROR)));        \
    } while(false)                                                      \

#endif // SSERVER_EXCEPTION_H_INCLUDED
