#ifndef SSERVER_CODE_H_INCLUDED
#define SSERVER_CODE_H_INCLUDED

enum Code { OK = 200, 
            BAD_REQUEST = 400,
            NOT_FOUND = 404,
            OVERLOADED = 405,
            NOT_ACCEPTABLE = 406,
            CONFLICT = 409,
            UNAVAILABLE = 503 };

template<typename T>
inline void notify(T& fd, Code code = OK) {
    REQUIRE(fd.write((char*)&code, sizeof(Code)) == sizeof(Code),
            "couldn't notify");
}

template<typename T>
inline void notify(T* fd, Code code = OK) {
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
