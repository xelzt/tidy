#pragma once

#include <unistd.h>


class InotifyDescriptor
{
private:
    int fd = -1;

public:
    InotifyDescriptor() = default;
    explicit InotifyDescriptor(int fd) : fd(fd) {};
    ~InotifyDescriptor();

    InotifyDescriptor(const InotifyDescriptor&) = delete;
    InotifyDescriptor& operator=(const InotifyDescriptor&) = delete;

    InotifyDescriptor(InotifyDescriptor&& other) noexcept : fd(other.fd)
    {
        other.fd = -1;
    }

    InotifyDescriptor& operator=(InotifyDescriptor&& other) noexcept
    {
        if (this != &other) {
            if (fd >= 0) {
                ::close(this->fd);
            }

            this->fd = other.fd;
            other.fd = -1;
        }

        return *this;
    }

    static InotifyDescriptor init(int flags);
    int* get();
};