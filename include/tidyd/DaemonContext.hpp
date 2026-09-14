#pragma once

#include <condition_variable>
#include <queue>
#include <string>
#include "common/InotifyDescriptor.hpp"
#include "common/TidyDescriptor.hpp"

enum class Op
{
    Stop,
    Tidy,
    NewFile
};

struct Job
{
    Op op;
    std::string path;
};

struct DaemonContext
{
    std::queue<Job> jobQueue;
    std::condition_variable cv;
    std::mutex operationMutex;

    InotifyDescriptor inotifyFd;
    TidyDescriptor socketFd;
};