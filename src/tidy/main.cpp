#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_NAME "/tmp/tidyd.socket"
#define BUFFER_SIZE 256

int main(int argc, char* argv[])
{
    struct sockaddr_un socket_data;
    int data_socket;
    char buffer[BUFFER_SIZE];
    bool read_flag = false;
    const std::string send_data = "ping";

    int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if(fd < 0)
    {
        std::cout << "Couldn't create socket !" << std::endl;
        return -1;
    }else {
        std::cout << "Socket created successfully!" << std::endl;
    }

    socket_data.sun_family = AF_LOCAL;
    strncpy(socket_data.sun_path, SOCKET_NAME, sizeof(socket_data.sun_path) - 1);

    if (connect(fd, reinterpret_cast<sockaddr*>(&socket_data), sizeof(socket_data))) {
        std::cout << "Error while connecting to socket!" << std::endl;
        return -1;
    }else {
        std::cout << "Connection to socket established - SUCCESS !" << std::endl;
    }

    if(write(fd, send_data.data(), send_data.size()) == -1)
    {
        perror("ERROR: Couldn't send data");
        close(fd);
        return 1;
    }

    const ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    if(n == -1)
    {
        perror("ERROR: Couldn't read the message");
        close(fd);
        return 1;
    }

    buffer[n] = '\0';
    std::cout << "Reply: " << buffer << std::endl;

    close(fd);
    return 0;
}