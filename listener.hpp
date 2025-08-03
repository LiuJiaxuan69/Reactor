#ifndef _LISTENER_HPP_
#define _LISTENER_HPP_ 1

#include <iostream>
#include <memory>
#include <thread>
#include "tcp.hpp"
#include "event_loop.hpp"
#include "log.hpp"

inline const uint16_t default_listen_port = 6349;

class Listener
{
public:
    Listener(uint16_t port = default_listen_port)
        : port_(port),
        sock_(new Sock())
    {
    }
    void Init()
    {
        sock_->Socket();
        sock_->Bind(port_);
        sock_->Listen();
        SetNonBlockOrDie(sock_->GetSockfd());
    }

    void Accepter(std::weak_ptr<Connection> conn)
    {
        auto connection = conn.lock();
        while (true)
        {
            int listen_sockfd = sock_->GetSockfd();
            struct sockaddr_in client;
            socklen_t len = sizeof(client);
            int client_sockfd = accept(listen_sockfd, (sockaddr *)&client, &len);
            if (client_sockfd == -1)
            {
                if (errno == EWOULDBLOCK)
                    break;
                else if (errno == EINTR)
                    continue;
                else
                {
                    lg(Error, "listening sock accept false, [%d]: %s", errno, strerror(errno));
                    continue;
                }
            }
            std::string client_ip;
            uint16_t client_port;
            Sock::GetAddrAndPort(client, client_ip, client_port);
            lg(Info, "accept a new client [%s: %d]", client_ip.c_str(), client_port);

            SetNonBlockOrDie(client_sockfd);

            // 加入到环形队列当中
            auto event_loop = connection->el.lock();
            ClientInf ci{
                .sockfd = client_sockfd,
                .client_ip = client_ip,
                .client_port = client_port
            };
            event_loop->rq_->Push(ci);

        }
    }

    int Fd() { return sock_->GetSockfd(); }

private:
    uint16_t port_;
    std::shared_ptr<Sock> sock_;
};

#endif