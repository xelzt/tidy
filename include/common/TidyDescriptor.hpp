#pragma once

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "common/definitions.hpp"

class TidyDescriptor{
private:
    int fd = -1;
    struct sockaddr_un socket_data;
public:
    TidyDescriptor(){};
    TidyDescriptor(const TidyDescriptor&) = delete;
    TidyDescriptor& operator=(const TidyDescriptor&) = delete;
    
    TidyDescriptor(TidyDescriptor&& other) noexcept 
    : fd(other.fd)
    , socket_data(other.socket_data)
    {
        other.fd = -1;
    }

    TidyDescriptor& operator=(TidyDescriptor&& other) noexcept
    {
        if (this != &other) {
            if (fd > 0) {
                ::close(fd);
            }

            fd = other.fd;
            socket_data = other.socket_data;
            other.fd = -1;
        }

        return *this;
    }

    TidyDescriptor(int fd);
    ~TidyDescriptor();

    static TidyDescriptor listen_on(){
        TidyDescriptor d;
        d.fd = socket(AF_LOCAL, SOCK_STREAM, 0);
        if(d.fd < 0)
        {
            std::cout << "Couldn't create socket !" << std::endl;
            perror("Couldn't create socket!");
        }else {
            std::cout << "Socket created successfully!" << std::endl;
        }

        memset(&d.socket_data, 0, sizeof(socket_data));
        d.socket_data.sun_family = AF_LOCAL;
        strncpy(d.socket_data.sun_path, SOCKET_NAME, sizeof(socket_data.sun_path) - 1);
        unlink(SOCKET_NAME);
        int ret = bind(d.fd, reinterpret_cast<const struct sockaddr*>(&d.socket_data), sizeof(d.socket_data));
        if (ret == -1) {
            perror("Error while binding socket!");
        }else {
            std::cout << "Socket binding - SUCCESS !" << std::endl;
        }

        return d;
    };

    static TidyDescriptor connect_to()
    {
        TidyDescriptor d;
        d.fd = socket(AF_LOCAL, SOCK_STREAM, 0);
        if(d.fd < 0)
        {
            std::cout << "Couldn't create socket !" << std::endl;
            perror("Couldn't create socket!");
        }else {
            std::cout << "Socket created successfully!" << std::endl;
        }

        memset(&d.socket_data, 0, sizeof(socket_data));
        d.socket_data.sun_family = AF_LOCAL;
        strncpy(d.socket_data.sun_path, SOCKET_NAME, sizeof(socket_data.sun_path) - 1);
        int ret = connect(d.fd, reinterpret_cast<const struct sockaddr*>(&d.socket_data), sizeof(d.socket_data));
        if (ret == -1) {
            perror("Error while connecting socket!");
        }else {
            std::cout << "Connected to socket - SUCCESS !" << std::endl;
        }

        return d;
    };

    int* get();
};