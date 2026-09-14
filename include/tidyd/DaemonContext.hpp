#pragma once

#include <atomic>
#include <condition_variable>
#include <queue>
#include <string>
#include <vector>
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

struct Rule
{
    std::string pattern;
    std::string path;
};

struct DaemonContext
{
    std::queue<Job> jobQueue;
    std::condition_variable cv;
    std::mutex operationMutex;
    std::atomic<bool> running{true};

    InotifyDescriptor inotifyFd;
    TidyDescriptor socketFd;

    std::vector<Rule> rules;
};