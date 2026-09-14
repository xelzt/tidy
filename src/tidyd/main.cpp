#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include "common/definitions.hpp"
#include "common/TidyDescriptor.hpp"
#include "common/InotifyDescriptor.hpp"
#include <condition_variable>
#include <fcntl.h>
#include <sys/inotify.h>
#include <poll.h>
#include "nlohmann/json_fwd.hpp"
#include "tidyd/DaemonContext.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

void runTidyMode(std::string filePath)
{
    std::cout << "File: " << filePath << "\n";
}

void runNewFileMode(std::string filePath, DaemonContext& context)
{
    fs::path name = filePath;
    std::string ext = name.extension();
    
    for (auto rule : context.rules) {
        if (ext == rule.pattern) {
            std::string srcPath = fs::path(DIRECTORY_WATCHER_PATH) / filePath;
            std::string dstPath = fs::path(rule.path) / filePath;

            fs::create_directories(rule.path);
            std::filesystem::rename(srcPath, dstPath);
            std::cout << "Moving: " << srcPath << " to: " << dstPath << "\n";
            return;
        }
    }

    std::cerr << "No rule for " << DIRECTORY_WATCHER_PATH + filePath << " file\n";
}

void runTidyWorker(DaemonContext& context)
{
    while (true) {
        std::unique_lock<std::mutex> lock(context.operationMutex);
        context.cv.wait(lock, [&] {return !context.jobQueue.empty();});

        Job job = context.jobQueue.front();
        context.jobQueue.pop();
        lock.unlock();

        if(job.op == Op::Tidy)
        {
            runTidyMode(job.path);
        }
        else if (job.op == Op::Stop) {
            break;
        }else if (job.op == Op::NewFile) {
            runNewFileMode(job.path, context);
        }
    }
}

void runIpcWatcher(DaemonContext& context)
{
    std::string response_data = "pong";
    char buffer[DAEMON_BUFFER_SIZE];
    int* fd = context.socketFd.get();

    int lis_ret = listen(*fd, 20);
    if (lis_ret == -1){
        std::cout << "Can't listen on socket!" << std::endl;
    }

    for(;;)
    {
        TidyDescriptor conn{accept(*fd, NULL, NULL)};
        if (*conn.get() < 0) {
            perror("Couln't accept socket data");
        }else {
            std::cout << "SUCCESS: Socket accepted" << std::endl;
        }

        for(;;)
        {
            ssize_t bytes_read = read(*conn.get(), buffer, sizeof(buffer) - 1);
            if(bytes_read < 0)
            {
                perror("ERROR: Couldn't read the buffer");
                break;
            }

            if(bytes_read == 0)
            {
                break;
            }

            buffer[bytes_read] = '\0';
            std::string_view cmd(buffer);

            if(cmd == "end")
            {
                response_data = "Daemon disabled";
                write(*conn.get(), response_data.data(), response_data.size());

                context.running = false;
                {
                    std::lock_guard<std::mutex> lock(context.operationMutex);
                    context.jobQueue.push(Job{Op::Stop});
                    context.inotifyFd.close();
                }
                context.cv.notify_all();
                return;
            }
            else if(cmd == "ping")
            {
                response_data = "pong";
            }
            else if (cmd == "status") {
                int pid = getpid();
                response_data = "PID: " + std::to_string(pid);
            }
            else if (cmd == "tidy") {
                {
                    response_data = "tidy";
                    std::lock_guard<std::mutex> lock(context.operationMutex);
                    Job tidyJob = Job{Op::Tidy};
                    context.jobQueue.push(tidyJob);
                }
                context.cv.notify_one();
            }

            std::cout << buffer << std::endl;

            write(*conn.get(), response_data.data(), response_data.size());
        }
    }
}

void runDirectoryWatcher(DaemonContext& context)
{
    alignas(inotify_event) char buffer[WATCHER_BUFFER_SIZER];
    int* descriptorPtr = context.inotifyFd.get();

    int wd = inotify_add_watch(*descriptorPtr, DIRECTORY_WATCHER_PATH, IN_CREATE | IN_MOVED_TO);
    if (wd < 0) {
        std::cout << "Problem occured while adding watcher!\n";
        return;
    }

    while (context.running.load()) 
    {
        pollfd pfd{};
        pfd.fd = *descriptorPtr;
        pfd.events = POLLIN;

        int pr = poll(&pfd, 1, 200);
        if (pr < 0) {
            std::cout << "Problem occured while polling inotify\n";
            return;
        }
        if (pr == 0) {
            continue;
        }
        if (!context.running.load()) {
            return;
        }

        int n = read(*descriptorPtr, buffer, WATCHER_BUFFER_SIZER);
        if (n < 0) {
            std::cout << "Problem occured while reading events to buffer\n";
            return;
        }

        for (char* p = buffer; p < buffer + n;) {
            auto* iEvent = reinterpret_cast<struct inotify_event*>(p);
            if (iEvent->len > 0) {
                std::cout << "Name: " << iEvent->name << " mask: " << iEvent->mask << "\n";
                {
                    std::lock_guard<std::mutex> lock(context.operationMutex);
                    context.jobQueue.push(Job{Op::NewFile, iEvent->name});
                }
                context.cv.notify_one();
            }

            p += sizeof(struct inotify_event) + iEvent->len;
        }
    }
}

int main(int argc, char* argv[])
{
    DaemonContext tidyDaemon;
    std::string config_path;
    for(int i = 1; i < argc; i++)
    {
        std::string argument = argv[i];
        if (argument == "--config") {
            if (i + 1 >= argc) {
                std::cerr << "tidyd: config flag requires path!\n";
                return 1;
            }
            config_path = argv[++i];
        }
        else if (argument == "--help") {
            std::cout << "--config -> Path to json file with rules\n";
            return 0;
        }else {
            std::cerr << "Incorrect option!\n";
            return 1;
        }
    }

    std::ifstream jsonFile(config_path);
    json data = json::parse(jsonFile);

    for(auto a : data.items())
    {
        tidyDaemon.rules.push_back(Rule{a.key(), a.value()});
    }

    tidyDaemon.inotifyFd = InotifyDescriptor::init(IN_CLOEXEC);
    tidyDaemon.socketFd = TidyDescriptor::listen_on();

    std::jthread tidyWorker(runTidyWorker, std::ref(tidyDaemon));
    std::jthread tidyWatcher(runIpcWatcher, std::ref(tidyDaemon));
    std::jthread tidyDirectoryWatcher(runDirectoryWatcher, std::ref(tidyDaemon));

    return 0;
}