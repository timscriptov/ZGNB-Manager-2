#ifndef SCOPED_FD_H_included
#define SCOPED_FD_H_included

#include <unistd.h>

class ScopedFd {
public:
    explicit ScopedFd(int fd) : fd(fd) {
    }

    ~ScopedFd() {
        close(fd);
    }

    int get() const {
        return fd;
    }

private:
    int fd;

    ScopedFd(const ScopedFd &);
    void operator=(const ScopedFd &);
};

#endif
