#ifndef _EVENT_LOOP_HPP_
#define _EVENT_LOOP_HPP_ 1

#include <iostream>
#include <unordered_map>
#include <memory>
#include <functional>
#include "tcp.hpp"
#include "epoll.hpp"
#include "log.hpp"
#include "common.hpp"
#include "ring_queue.hpp"
#include "timer_manager.hpp"
#include "connection.hpp"

// 前向声明
class Connection;      // 连接类
class EventLoop;       // 事件循环类
struct ClientInf;      // 客户端信息结构体
class Timer;          // 定时器类
class TimerManager;   // 定时器管理类

// 定义任务类型：接收环形队列和事件循环的弱指针
using task_t = std::function<void(std::weak_ptr<RingQueue<ClientInf>>, std::weak_ptr<EventLoop>)>;

inline constexpr size_t max_fd = 1 << 10;  // 最大文件描述符数量
inline const uint16_t default_port = 7777; // 默认端口号
inline thread_local char buffer[1024];     // 线程本地接收缓冲区

// 客户端信息结构体
struct ClientInf{
    int sockfd;             // 客户端socket文件描述符
    std::string client_ip;  // 客户端IP地址
    uint16_t client_port;   // 客户端端口号
};

// 事件循环类，继承enable_shared_from_this以支持shared_from_this()
class EventLoop: public std::enable_shared_from_this<EventLoop>
{
public:
    // 构造函数
    // rq: 环形队列指针，用于任务分发（可为空）
    // OnMessage: 消息到达时的回调函数
    // TaskPush: 任务推送函数（可为空）
    EventLoop(std::shared_ptr<RingQueue<ClientInf>> rq = nullptr, 
             func_t OnMessage = nullptr, 
             task_t TaskPush = nullptr)
    :epoller_(new Epoll()),      // 初始化epoll实例
    OnMessage_(OnMessage),       // 设置消息回调
    TaskPush_(TaskPush),         // 设置任务推送函数
    rq_(rq),                     // 设置环形队列
    tm_(new TimerManager())      // 初始化定时器管理器
    {}
    
    // 添加新连接到事件循环
    void AddConnection(int sock,           // socket文件描述符
                      uint32_t event,      // 监听的事件类型
                      func_t recv_cb,     // 读回调
                      func_t send_cb,      // 写回调
                      func_t except_cb,   // 异常回调
                      const std::string &ip = "0.0.0.0",  // IP地址
                      uint16_t port = 0,   // 端口号
                      bool is_listensock = false)  // 是否监听socket
    {
        // 创建新连接对象
        std::shared_ptr<Connection> new_connect(new Connection(sock));
        new_connect->el = shared_from_this();  // 设置所属事件循环
        new_connect->recv_cb = recv_cb;       // 设置读回调
        new_connect->send_cb = send_cb;        // 设置写回调
        new_connect->except_cb = except_cb;    // 设置异常回调
        new_connect->ip_ = ip;                 // 设置IP
        new_connect->port_ = port;             // 设置端口

        // 如果不是监听socket，则加入定时器管理
        if(!is_listensock) tm_->Push(new_connect);
        // 添加到连接映射表
        connections_[sock] = new_connect;

        // 将socket添加到epoll监听
        epoller_->EpollCtl(EPOLL_CTL_ADD, sock, event);
    }
    
    // 从连接接收数据
    void Recv(std::weak_ptr<Connection> connect)
    {
        auto connection = connect.lock();  // 获取连接的共享指针
        int sock = connection->Sockfd();   // 获取socket文件描述符
        
        while(true)
        {
            // 接收数据
            ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if(n > 0)  // 成功接收到数据
            {
                buffer[n] = 0;  // 添加字符串结束符
                connection->AppendInBuffer(buffer);  // 将数据追加到输入缓冲区
                lg(Debug, "thread-%d, recv message from client: %s", pthread_self(),buffer);
            }
            else if(n == 0)  // 客户端关闭连接
            {
                lg(Info, "client [%s: %d] quit", connection->ip_.c_str(), connection->port_);
                connection->except_cb(connection);  // 调用异常回调
                return;
            }
            else  // 接收出错
            {
                if(errno == EWOULDBLOCK) break;  // 非阻塞模式下无数据可读
                else if(errno == EINTR) continue;  // 被信号中断，继续读取
                else  // 其他错误
                {
                    lg(Error, "recv from client [%s: %d] false", connection->ip_.c_str(), connection->port_);
                    connection->except_cb(connection);
                    return;
                }
            }
        }
        
        // 如果有消息处理回调，则调用
        if(OnMessage_)
            OnMessage_(connection);
    }
    
    // 向连接发送数据
    void Send(std::weak_ptr<Connection> connect)
    {
        auto connection = connect.lock();  // 获取连接的共享指针
        std::string &outbuffer = connection->OutBuffer();  // 获取输出缓冲区
        
        while(true)
        {
            // 发送数据
            ssize_t n = send(connection->Sockfd(), outbuffer.c_str(), outbuffer.size(), 0);
            if(n > 0)  // 成功发送部分数据
            {
                outbuffer.erase(0, n);  // 从缓冲区移除已发送数据
                if(outbuffer.empty()) break;  // 缓冲区已空，发送完成
            }
            else if(n == 0) return;  // 发送0字节，连接可能已关闭
            else  // 发送出错
            {
                if(errno == EWOULDBLOCK) break;  // 发送缓冲区满
                else if(errno == EINTR) continue;  // 被信号中断，继续发送
                else  // 其他错误
                {
                    lg(Error, "send to client [%s: %d] false", connection->ip_.c_str(), connection->port_);
                    connection->except_cb(connection);
                    return;
                }
            }
            
            // 根据缓冲区状态调整epoll监听事件
            if(!outbuffer.empty() && !connection->write_care_)  // 缓冲区非空且未关注写事件
            {
                // 添加对写事件的监听（边缘触发模式）
                epoller_->EpollCtl(EPOLL_CTL_MOD,connection->Sockfd(), EPOLLIN | EPOLLOUT | EPOLLET);
                connection->write_care_ = true;
            }
            else if(outbuffer.empty() && connection->write_care_)  // 缓冲区空且关注了写事件
            {
                // 取消对写事件的监听
                epoller_->EpollCtl(EPOLL_CTL_MOD,connection->Sockfd(), EPOLLIN | EPOLLET);
                connection->write_care_ = false;
            }
        }
    }
    
    // 处理连接异常
    void Except(std::weak_ptr<Connection> connect)
    {
        auto connection = connect.lock();  // 获取连接的共享指针
        int fd = connection->Sockfd();     // 获取socket文件描述符
        
        lg(Warning, "client [%s: %d] handler exception", connection->ip_.c_str(), connection->port_);

        // 从epoll中删除该socket
        epoller_->EpollCtl(EPOLL_CTL_DEL, fd, 0);
        lg(Debug, "client [%s: %d] close done", connection->ip_.c_str(), connection->port_);
        
        close(fd);  // 关闭socket
        connections_.erase(fd);  // 从连接表中移除
        tm_->LazyDelete(fd);     // 从定时器中延迟删除
    }
    
    // 事件分发器
    void DisPatcher()
    {
        // 等待事件发生，返回就绪事件数量
        int n = epoller_->EpollWait(recvs, max_fd);
        for(int i = 0; i < n; ++i)
        {
            uint32_t events = recvs[i].events;  // 事件类型
            int sockfd = recvs[i].data.fd;      // 发生事件的socket
            
            // 将错误事件转换为读写事件处理
            if(events & (EPOLLERR | EPOLLHUP)) events |= (EPOLLIN | EPOLLOUT);

            // 处理读事件
            if(events & EPOLLIN && connections_[sockfd]->recv_cb) 
            {
                connections_[sockfd]->recv_cb(connections_[sockfd]);
                tm_->UpdateTime(sockfd);  // 更新该连接的定时器
            }
            // 处理写事件
            if(events & EPOLLOUT && connections_[sockfd]->send_cb) 
            {
                connections_[sockfd]->send_cb(connections_[sockfd]);
                tm_->UpdateTime(sockfd);  // 更新该连接的定时器
            }
        }
    }
    
    // 检查过期连接
    void Expired_check()
    {
        while(tm_->IsTopExpired())  // 检查堆顶定时器是否过期
        {
            auto top_time = tm_->GetTop()->connect;  // 获取过期连接
            int sockfd = top_time->Sockfd();         // 获取socket文件描述符
            
            // 如果连接仍然存在（未被客户端主动关闭）
            if(connections_.find(sockfd) != connections_.end())
                connections_[sockfd]->except_cb(top_time);  // 调用异常回调处理
        }
    }
    
    // 主事件循环
    void Loop()
    {
        while(true)
        {
            // 如果有任务推送函数，则执行
            if(TaskPush_) TaskPush_(rq_, shared_from_this());
            
            DisPatcher();      // 事件分发
            Expired_check();   // 检查过期连接
        }
    }

public:
    std::shared_ptr<RingQueue<ClientInf>> rq_;  // 环形队列，用于任务分发

private:
    std::unordered_map<int, std::shared_ptr<Connection>> connections_;  // 连接表
    std::shared_ptr<Epoll> epoller_;             // epoll实例
    std::shared_ptr<TimerManager> tm_;           // 定时器管理器
    struct epoll_event recvs[max_fd];            // epoll事件数组
    func_t OnMessage_;                          // 消息到达回调
    task_t TaskPush_;                           // 任务推送函数
};

#endif