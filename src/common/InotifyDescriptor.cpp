#include "common/InotifyDescriptor.hpp"
#include <unistd.h>
#include <sys/inotify.h>

InotifyDescriptor::InotifyDescriptor(int fd)
{
    this->fd = fd;
}

InotifyDescriptor::~InotifyDescriptor()
{
    if (fd >= 0) {
        ::close(this->fd);
        fd = -1;
    }
}

InotifyDescriptor InotifyDescriptor::init(int flags)
{
    return InotifyDescriptor(inotify_init1(flags));
}

int* InotifyDescriptor::get()
{
    return &this->fd;
}