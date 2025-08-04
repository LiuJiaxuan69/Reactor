#ifndef _LISTENER_HPP_
#define _LISTENER_HPP_ 1

#include <iostream>
#include <memory>
#include <thread>
#include "tcp.hpp"         // TCP socket封装
#include "event_loop.hpp"  // 事件循环
#include "log.hpp"         // 日志系统

inline const uint16_t default_listen_port = 6349; // 默认监听端口

/**
 * @brief TCP监听器类
 * @note 通过环形队列实现与I/O线程的解耦
 */
class Listener
{
public:
    /**
     * @brief 构造函数
     * @param port 监听端口号
     */
    Listener(uint16_t port = default_listen_port)
        : port_(port),
        sock_(new Sock()) // 创建TCP socket封装对象
    {
    }

    /**
     * @brief 初始化监听socket
     */
    void Init()
    {
        sock_->Socket();  // 创建socket
        sock_->Bind(port_); // 绑定端口
        sock_->Listen();    // 开始监听
        SetNonBlockOrDie(sock_->GetSockfd()); // 设置为非阻塞模式
    }

    /**
     * @brief 接受客户端连接的主循环
     * @param conn 持有EventLoop引用的Connection对象
     */
    void Accepter(std::weak_ptr<Connection> conn)
    {
        auto connection = conn.lock();
        while (true)
        {
            int listen_sockfd = sock_->GetSockfd();
            struct sockaddr_in client;
            socklen_t len = sizeof(client);
            // 非阻塞accept
            int client_sockfd = accept(listen_sockfd, (sockaddr *)&client, &len);
            
            if (client_sockfd == -1)
            {
                if (errno == EWOULDBLOCK)
                    break;  // 无新连接时退出循环
                else if (errno == EINTR)
                    continue; // 被信号中断则重试
                else
                {
                    // 记录accept错误日志
                    lg(Error, "listening sock accept false, [%d]: %s", errno, strerror(errno));
                    continue;
                }
            }
            
            // 获取客户端地址信息
            std::string client_ip;
            uint16_t client_port;
            Sock::GetAddrAndPort(client, client_ip, client_port);
            lg(Info, "accept a new client [%s: %d]", client_ip.c_str(), client_port);

            // 设置客户端socket为非阻塞
            SetNonBlockOrDie(client_sockfd);

            // 通过EventLoop的环形队列传递连接信息
            auto event_loop = connection->el.lock();
            ClientInf ci{
                .sockfd = client_sockfd,
                .client_ip = client_ip,
                .client_port = client_port
            };
            event_loop->rq_->Push(ci); // 入队操作
        }
    }

    /// 获取监听socket文件描述符
    int Fd() { return sock_->GetSockfd(); }

private:
    uint16_t port_;             // 监听端口
    std::shared_ptr<Sock> sock_; // TCP socket封装对象
};

#endif