#include "common/InotifyDescriptor.hpp"
#include <unistd.h>
#include <sys/inotify.h>


InotifyDescriptor::~InotifyDescriptor()
{
    this->close();
}

InotifyDescriptor InotifyDescriptor::init(int flags)
{
    return InotifyDescriptor(inotify_init1(flags));
}

int* InotifyDescriptor::get()
{
    return &this->fd;
}

void InotifyDescriptor::close()
{
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}