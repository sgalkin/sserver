#ifndef SSERVER_CODE_H_INCLUDED
#define SSERVER_CODE_H_INCLUDED

#include <map>
#include <string>
#include <stdexcept>

class Codes {
public:
    enum Code { OK = 200, 
                BAD_REQUEST = 400,
                NOT_FOUND = 404,
                OVERLOADED = 405,
                NOT_ACCEPTABLE = 406,
                CONFLICT = 409,
                UNAVAILABLE = 503 };

    static const std::string& string(Code code) {
        return strings_.at(code);
    }

private:
    static std::map<Code, std::string> strings_;
};

typedef Codes::Code Code;

class BaseProtocolError : public std::runtime_error {
public:
    explicit BaseProtocolError(Codes::Code code) :
        std::runtime_error(Codes::string(code)) {} // have copy of string here, optimize if need
};

template<Code code>
class ProtocolError : public BaseProtocolError {
public:
    ProtocolError() : BaseProtocolError(code) {}
};

typedef ProtocolError<Codes::BAD_REQUEST> BadRequest;
typedef ProtocolError<Codes::NOT_FOUND> NotFound;
typedef ProtocolError<Codes::OVERLOADED> Overloaded;
typedef ProtocolError<Codes::NOT_ACCEPTABLE> NotAcceptable;
typedef ProtocolError<Codes::CONFLICT> Conflict;
typedef ProtocolError<Codes::UNAVAILABLE> Unavailable;

template<typename T>
inline void notify(T& fd, Code code = Codes::OK) {
    REQUIRE(fd.write((char*)&code, sizeof(Code)) == sizeof(Code),
            "couldn't notify");
}

template<typename T>
inline void notify(T* fd, Code code = Codes::OK) {
    REQUIRE(fd, "NULL pointer in notify");
    notify(*fd, code);
}

template<typename T>
inline Code suppress(T& fd) {
    Code code;
    REQUIRE(fd.read((char*)&code, sizeof(Code)) == sizeof(Code),
            "couldn't get notification");
    return code;
}

template<typename T>
inline Code suppress(T* fd) {
    REQUIRE(fd, "NULL pointer in suppress");
    return suppress(*fd);
}

#endif // SSERVER_CODE_H_INCLUDED
