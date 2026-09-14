#include <cstddef>
#include <cstdio>
#include <cstring>
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

struct
{
    std::queue<Job> jobQueue;
    std::condition_variable cv;
    std::mutex operationMutex;
}tidyDaemonThread;

void runTidyMode()
{
    std::cout << "Tidy inside Tidy Daemon\n";
}

void runTidyWorker()
{
    while (true) {
        std::unique_lock<std::mutex> lock(tidyDaemonThread.operationMutex);
        tidyDaemonThread.cv.wait(lock, [&] {return !tidyDaemonThread.jobQueue.empty();});

        Job job = tidyDaemonThread.jobQueue.front();
        tidyDaemonThread.jobQueue.pop();
        lock.unlock();

        if(job.op == Op::Tidy)
        {
            runTidyMode();
        }
        else if (job.op == Op::Stop) {
            break;
        }else if (job.op == Op::NewFile) {
            std::cout << "Moving file: " << job.path << "\n";
        }
    }
}

void runIpcWatcher()
{
    std::string response_data = "pong";
    char buffer[DAEMON_BUFFER_SIZE];
    TidyDescriptor tidy_daemon = TidyDescriptor::listen_on();
    int* fd = tidy_daemon.get();

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

            if(cmd == "END")
            {
                {
                    std::lock_guard<std::mutex> lock(tidyDaemonThread.operationMutex);
                    Job stopJob = Job{Op::Stop};
                    tidyDaemonThread.jobQueue.push(stopJob);
                }
                tidyDaemonThread.cv.notify_one();
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
                    std::lock_guard<std::mutex> lock(tidyDaemonThread.operationMutex);
                    Job tidyJob = Job{Op::Tidy};
                    tidyDaemonThread.jobQueue.push(tidyJob);
                }
                tidyDaemonThread.cv.notify_one();
            }

            std::cout << buffer << std::endl;

            write(*conn.get(), response_data.data(), response_data.size());
        }
    }
}

void runDirectoryWatcher()
{
    int* descriptorPtr = nullptr;
    alignas(inotify_event) char buffer[WATCHER_BUFFER_SIZER];
    InotifyDescriptor inotifyWatcher = InotifyDescriptor::init(IN_CLOEXEC);
    descriptorPtr = inotifyWatcher.get();

    int wd = inotify_add_watch(*descriptorPtr, DIRECTORY_WATCHER_PATH, IN_CREATE | IN_MOVED_TO);
    if (wd < 0) {
        std::cout << "Problem occured while adding watcher!\n";
        return;
    }

    while (true) {
        int n = read(*descriptorPtr, buffer, WATCHER_BUFFER_SIZER);
        if (n < 0) {
            std::cout << "Problem occured while reading events to buffer\n";
            return;
        }

        for(char* p = buffer; p < buffer + n;)
        {
            auto* iEvent = reinterpret_cast<struct inotify_event*>(p);
            if (iEvent->len > 0) {
                std::cout << "Name: " << iEvent->name << " mask: " << iEvent->mask << "\n";
                {
                    std::lock_guard<std::mutex> lock(tidyDaemonThread.operationMutex);
                    Job newFileJob{Op::NewFile, iEvent->name};
                    tidyDaemonThread.jobQueue.push(newFileJob);
                }
                tidyDaemonThread.cv.notify_one();
            }

            p += sizeof(struct inotify_event) + iEvent->len;
            
        }
    }
}

int main(int argc, char* argv[])
{
    std::jthread tidyWorker(runTidyWorker);
    std::jthread tidyWatcher(runIpcWatcher);
    std::jthread tidyDirectoryWatcher(runDirectoryWatcher);

    return 0;
}