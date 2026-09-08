#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "common/TidyDescriptor.hpp"
#include "common/definitions.hpp"

int main(int argc, char* argv[])
{
    std::string response_data = "pong";
    int data_socket;
    char buffer[DAEMON_BUFFER_SIZE];
    bool read_flag = false;
    TidyDescriptor tidy_daemon = TidyDescriptor::listen_on();
    int* fd = tidy_daemon.get();

    int lis_ret = listen(*fd, 20);
    if (lis_ret == -1){
        std::cout << "Can't listen on socket!" << std::endl;
        return -1;
    }

    for(;;)
    {
        TidyDescriptor conn{accept(*fd, NULL, NULL)};
        if (*conn.get() < 0) {
            perror("Couln't accept socket data");
            return -1;
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
                read_flag = true;
                break;
            }

            if(strncmp(buffer, "ping", 4) == 0)
            {
                response_data = "pong";
            }
            else if (strncmp(buffer, "status", 6) == 0) {
                int pid = getpid();
                response_data = "PID: " + std::to_string(pid);
            }

            std::cout << buffer << std::endl;

            write(*conn.get(), response_data.data(), response_data.size());
        }

        close(data_socket);
        if(read_flag)
        {
            std::cout << "Closing connection\n";
            break;
        }
    }

    return 0;
}