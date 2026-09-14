#include "common/TidyDescriptor.hpp"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <unordered_map>

#define HELP_MODE 0
#define STATUS_MODE 1
#define PING_MODE 2
#define TIDY_MODE 3
#define END_MODE 4
#define DEFAULT_MODE 5

std::unordered_map<std::string, int> modes = {
    {"help", 0},
    {"status", 1},
    {"ping", 2},
    {"tidy", 3},
    {"end", 4},
    {"default", DEFAULT_MODE},
};

int map_args_to_mode(std::string mode_name)
{
    if (modes.contains(mode_name))
    {
        return modes.at(mode_name);
    }

    return modes.at("default");
}

int handle_operation(int fd, int mode)
{
    std::string send_data = "";
    switch (mode) {
        case PING_MODE:
            send_data = "ping";
            break;
        case STATUS_MODE:
            send_data = "status";
            break;
        case TIDY_MODE:
            send_data = "tidy";
            break;
        case END_MODE:
            send_data = "END";
            break;
        default:
            send_data = "ping";
            break;
    }

    char buffer[CLIENT_BUFFER_SIZE];
    if(write(fd, send_data.data(), send_data.size()) == -1)
    {
        perror("ERROR: Couldn't send data");
        close(fd);
        return 1;
    }

    const ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    if(n == -1)
    {
        perror("ERROR: Couldn't read the message. Daemon DOWN");
        close(fd);
        return 1;
    }

    buffer[n] = '\0';
    std::cout << "Reply: " << buffer << std::endl;

    return 0;
}

int handle_help()
{
    std::cout << "Arguments:\n";
    std::cout << "status - Returns the PID of tidy daemon process\n";
    std::cout << "ping - Ping the daemon which returns response if awake\n";

    return 0;
}

int main(int argc, char* argv[])
{
    TidyDescriptor td = TidyDescriptor::connect_to();
    int* fd = td.get();

    int mode = argc > 1 ? map_args_to_mode(static_cast<std::string>(argv[1])) : DEFAULT_MODE;
    switch (mode) {
        case HELP_MODE:
            handle_help();
            break;
        case PING_MODE:
        case STATUS_MODE:
        case TIDY_MODE:
        case END_MODE:
            handle_operation(*fd, mode);
            break;
        default:
            break;
    }

    return 0;
}