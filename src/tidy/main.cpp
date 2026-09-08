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

#define SOCKET_NAME "/tmp/tidyd.socket"
#define BUFFER_SIZE 256

#define HELP_MODE 0
#define STATUS_MODE 1
#define PING_MODE 2
#define DEFAULT_MODE 3

std::unordered_map<std::string, int> modes = {
    {"help", 0},
    {"status", 1},
    {"ping", 2},
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
        default:
            send_data = "ping";
            break;
    }

    char buffer[BUFFER_SIZE];
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

void setup(int* fd, struct sockaddr_un* socket_data)
{
    *fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if(*fd < 0)
    {
        std::cout << "Couldn't create socket !" << std::endl;
    }
    else
    {
        std::cout << "Socket created successfully!" << std::endl;
    }

    socket_data->sun_family = AF_LOCAL;
    strncpy(socket_data->sun_path, SOCKET_NAME, sizeof(socket_data->sun_path) - 1);

    if (connect(*fd, reinterpret_cast<sockaddr*>(socket_data), sizeof(*socket_data))) {
        std::cout << "Error while connecting to socket!" << std::endl;
    }else {
        std::cout << "Connection to socket established - SUCCESS !" << std::endl;
    }
}

void teardown(int* fd)
{
    close(*fd);
}

int main(int argc, char* argv[])
{
    int fd;
    struct sockaddr_un socket_data;

    int mode = argc > 1 ? map_args_to_mode(static_cast<std::string>(argv[1])) : DEFAULT_MODE;
    switch (mode) {
        case HELP_MODE:
            handle_help();
            break;
        case PING_MODE:
            setup(&fd, &socket_data);
            handle_operation(fd, mode);
            teardown(&fd);
            break;
        case STATUS_MODE:
            setup(&fd, &socket_data);
            handle_operation(fd, mode);
            teardown(&fd);
            break;
        default:
            break;
    }

    return 0;
}