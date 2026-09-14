#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include "common/TidyDescriptor.hpp"
#include "common/definitions.hpp"
#include <condition_variable>

enum class Op
{
    Stop,
    Tidy
};

struct
{
    std::queue<Op> operationQueue;
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
        tidyDaemonThread.cv.wait(lock, [&] {return !tidyDaemonThread.operationQueue.empty();});

        Op operationType = tidyDaemonThread.operationQueue.front();
        tidyDaemonThread.operationQueue.pop();
        lock.unlock();

        if(operationType == Op::Tidy)
        {
            runTidyMode();
        }
        else if (operationType == Op::Stop) {
            break;
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

            if(strncmp(buffer, "END", 3) == 0)
            {
                {
                    std::lock_guard<std::mutex> lock(tidyDaemonThread.operationMutex);
                    tidyDaemonThread.operationQueue.push(Op::Stop);
                }
                tidyDaemonThread.cv.notify_one();
                return;
            }

            if(strncmp(buffer, "ping", 4) == 0)
            {
                response_data = "pong";
            }
            else if (strncmp(buffer, "status", 6) == 0) {
                int pid = getpid();
                response_data = "PID: " + std::to_string(pid);
            }
            else if (strncmp(buffer, "tidy", 4) == 0) {
                {
                    response_data = "tidy";
                    std::lock_guard<std::mutex> lock(tidyDaemonThread.operationMutex);
                    tidyDaemonThread.operationQueue.push(Op::Tidy);
                }
                tidyDaemonThread.cv.notify_one();
            }

            std::cout << buffer << std::endl;

            write(*conn.get(), response_data.data(), response_data.size());
        }
    }
}

int main(int argc, char* argv[])
{
    std::jthread tidyWorker(runTidyWorker);
    std::jthread tidyWatcher(runIpcWatcher);

    return 0;
}