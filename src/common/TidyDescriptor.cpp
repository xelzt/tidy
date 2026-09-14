#include "common/TidyDescriptor.hpp"
#include <cstdio>
#include <cstring>
#include <unistd.h>

TidyDescriptor::TidyDescriptor(int fd)
{
    this->fd = fd;
}

TidyDescriptor::~TidyDescriptor()
{
    if(fd >= 0)
    {
        ::close(this->fd);
        this->fd = -1;
    }
}

int* TidyDescriptor::get()
{
    return &this->fd;
}