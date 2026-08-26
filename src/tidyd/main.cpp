#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_NAME "/tmp/tidyd.socket"
#define BUFFER_SIZE 16

int main(int argc, char* argv[])
{
    struct sockaddr_un socket_data;
    std::string response_data = "pong";
    int data_socket;
    char buffer[BUFFER_SIZE];
    bool read_flag = false;

    int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if(fd < 0)
    {
        std::cout << "Couldn't create socket !" << std::endl;
        return -1;
    }else {
        std::cout << "Socket created successfully!" << std::endl;
    }

    memset(&socket_data, 0, sizeof(socket_data));
    socket_data.sun_family = AF_LOCAL;
    strncpy(socket_data.sun_path, SOCKET_NAME, sizeof(socket_data.sun_path) - 1);
    unlink(SOCKET_NAME);

    int ret = bind(fd, reinterpret_cast<const struct sockaddr*>(&socket_data), sizeof(socket_data));

    if (ret == -1) {
        std::cout << "Error while binding socket!" << std::endl;
        return -1;
    }else {
        std::cout << "Socket binding - SUCCESS !" << std::endl;
    }

    int lis_ret = listen(fd, 20);
    if (lis_ret == -1){
        std::cout << "Can't listen on socket!" << std::endl;
        return -1;
    }

    for(;;)
    {
        data_socket = accept(fd, NULL, NULL);
        if (data_socket == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }else {
            std::cout << "SUCCESS: Socket accepted" << std::endl;
        }

        for(;;)
        {
            ssize_t bytes_read = read(data_socket, buffer, sizeof(buffer) - 1);
            if(bytes_read < 0)
            {
                perror("ERROR: Couldn't read the buffer");
                break;
            }

            buffer[bytes_read] = '\0';

            if(strncmp(buffer, "END", 3) == 0)
            {
                read_flag = true;
                break;
            }

            std::cout << buffer << std::endl;

            write(data_socket, response_data.data(), response_data.size());
        }

        if(read_flag)
        {
            break;
        }
    }

    close(fd);
    return 0;
}