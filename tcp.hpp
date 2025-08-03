#ifndef _TCP_HPP_
#define _TCP_HPP_ 1

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include "log.hpp"

enum
{
    socket_error = 1,
    bind_error,
    listen_error,
    connect_error,
};

inline thread_local char addr_buffer[1024];

class Sock
{
public:
    Sock() {}
    void Socket()
    {
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ == -1)
        {
            perror("socket");
            exit(socket_error);
        }
    }
    void Bind(int port)
    {
        struct sockaddr_in local;
        bzero(&local, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = INADDR_ANY;
        local.sin_port = htons(port);
        int n = bind(sockfd_, (struct sockaddr *)(&local), sizeof(local));
        if (n != 0)
        {
            perror("bind");
            exit(bind_error);
        }
    }
    void Listen(int backlog = 10)
    {
        int n = listen(sockfd_, backlog);
        if (n == -1)
        {
            perror("listen");
            exit(listen_error);
        }
    }
    void Connect(sockaddr_in &server)
    {
        int n = connect(sockfd_, (struct sockaddr *)&server, sizeof(server));
        if (n == -1)
        {
            perror("connect");
            exit(connect_error);
        }
    }
    int GetSockfd(){ return sockfd_; }

public:
    static void GetAddrAndPort(struct sockaddr_in &addr_in, std::string &addr, uint16_t &port)
    {
        port = ntohs(addr_in.sin_port);
        inet_ntop(AF_INET, &addr_in.sin_addr, addr_buffer, sizeof(addr_buffer) - 1);
        addr = addr_buffer;
    }

private:
    int sockfd_;
};

#endif