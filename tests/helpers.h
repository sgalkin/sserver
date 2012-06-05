#ifndef SSERVER_TEST_HELPERS_H_INCLUDED
#define SSERVER_TEST_HELPERS_H_INCLUDED

#include <fcntl.h>
#include <unistd.h>

int count_descriptors() {
    int count = 0;
    for(int i = 0; i < sysconf(_SC_OPEN_MAX); ++i) {
        if(fcntl(i, F_GETFL, 0) != -1) ++count;
    }
    return count;
}

#endif // SSERVER_TEST_HELPERS_H_INCLUDED
