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

class Connection;
class EventLoop;
struct ClientInf;
class Timer;
class TimerManager;

using task_t = std::function<void(std::weak_ptr<RingQueue<ClientInf>>, std::weak_ptr<EventLoop>)>;

inline constexpr size_t max_fd = 1 << 10;

inline const uint16_t default_port = 7777;

inline thread_local char buffer[1024];


struct ClientInf{
    int sockfd;
    std::string client_ip;
    uint16_t client_port;
};



class EventLoop: public std::enable_shared_from_this<EventLoop>
{
public:
    // 当 Task_Push 为空时，即代表不从上层循环队列当中获取任务，即主任务循环器的选择，因为主任务循环器只负责任务 Push，不参与 Pop
    EventLoop(std::shared_ptr<RingQueue<ClientInf>> rq = nullptr, func_t OnMessage = nullptr, task_t TaskPush = nullptr)
    :epoller_(new Epoll()),
    OnMessage_(OnMessage),
    TaskPush_(TaskPush),
    rq_(rq),
    tm_(new TimerManager())
    {}
    void AddConnection(int sock, uint32_t event, func_t recv_cb, func_t send_cb, func_t except_cb,
                       const std::string &ip = "0.0.0.0", uint16_t port = 0, bool is_listensock = false)
    {
        std::shared_ptr<Connection> new_connect(new Connection(sock));
        new_connect->el = shared_from_this();
        new_connect->recv_cb = recv_cb;
        new_connect->send_cb = send_cb;
        new_connect->except_cb = except_cb;
        new_connect->ip_ = ip;
        new_connect->port_ = port;

        if(!is_listensock) tm_->Push(new_connect);
        connections_[sock] = new_connect;

        
        epoller_->EpollCtl(EPOLL_CTL_ADD, sock, event);
    }
    void Recv(std::weak_ptr<Connection> connect)
    {
        auto connection = connect.lock();
        int sock = connection->Sockfd();
        while(true)
        {
            ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if(n > 0)
            {
                buffer[n] = 0;
                connection->AppendInBuffer(buffer);
                lg(Debug, "thread-%d, recv message from client: %s", pthread_self(),buffer);
            }else if(n == 0){
                lg(Info, "client [%s: %d] quit", connection->ip_.c_str(), connection->port_);
                connection->except_cb(connection);
                return;
            }else{
                if(errno == EWOULDBLOCK) break;
                else if(errno == EINTR) continue;
                else {
                    lg(Error, "recv from client [%s: %d] false", connection->ip_.c_str(), connection->port_);
                    connection->except_cb(connection);
                    return;
                }
            }
        }
        if(OnMessage_)
        OnMessage_(connection);
    }
    void Send(std::weak_ptr<Connection> connect)
    {
        auto connection = connect.lock();

        std::string &outbuffer = connection->OutBuffer();
        while(true)
        {
            ssize_t n = send(connection->Sockfd(), outbuffer.c_str(), outbuffer.size(), 0);
            if(n > 0)
            {
                outbuffer.erase(0, n);
                if(outbuffer.empty()) break;
            }
            else if(n == 0) return;
            else{
                if(errno == EWOULDBLOCK) break;
                else if(errno == EINTR) continue;
                else{
                    lg(Error, "send to client [%s: %d] false", connection->ip_.c_str(), connection->port_);
                    connection->except_cb(connection);
                    return;
                }
            }
            // 写缓冲满
            if(!outbuffer.empty() && !connection->write_care_)
            {
                // 开启写时间关心
                epoller_->EpollCtl(EPOLL_CTL_MOD,connection->Sockfd(), EPOLLIN | EPOLLOUT | EPOLLET);
            }else if(outbuffer.empty() && connection->write_care_){
                // 关闭写事件特别关心
                epoller_->EpollCtl(EPOLL_CTL_MOD,connection->Sockfd(), EPOLLIN | EPOLLET);
            }
        }
    }
    void Except(std::weak_ptr<Connection> connect)
    {
        auto connection = connect.lock();

        int fd = connection->Sockfd();
        lg(Warning, "client [%s: %d] handler exception", connection->ip_.c_str(), connection->port_);

        epoller_->EpollCtl(EPOLL_CTL_DEL, fd, 0);
        lg(Debug, "client [%s: %d] close done", connection->ip_.c_str(), connection->port_);
        close(fd);
        connections_.erase(fd);
    }
    void DisPatcher()
    {
        int n = epoller_->EpollWait(recvs, max_fd);
        for(int i = 0; i < n; ++i)
        {
            uint32_t events = recvs[i].events;
            int sockfd = recvs[i].data.fd;
            // 异常事件转化为读写问题处理
            if(events & (EPOLLERR | EPOLLHUP)) events |= (EPOLLIN | EPOLLOUT);

            if(events & EPOLLIN && connections_[sockfd]->recv_cb) {
                connections_[sockfd]->recv_cb(connections_[sockfd]);
                tm_->UpdateTime(sockfd);
            }
            if(events & EPOLLOUT && connections_[sockfd]->send_cb) {
                connections_[sockfd]->send_cb(connections_[sockfd]);
                tm_->UpdateTime(sockfd);
            }

        }
    }
    void Expired_check()
    {
        while(tm_->IsTopExpired()){
            auto top_time = tm_->GetTop()->connect;
            int sockfd = top_time->Sockfd();
            tm_->DeleteTop();
            // 找不到说明是客户端主动释放的
            if(connections_.find(sockfd) != connections_.end())
            connections_[sockfd]->except_cb(top_time);
        }
    }
    void Loop()
    {
        while(true)
        {
            if(TaskPush_) TaskPush_(rq_, shared_from_this());
            DisPatcher();
            Expired_check();
        }
    }
public:
    std::shared_ptr<RingQueue<ClientInf>> rq_;
private:
    std::unordered_map<int, std::shared_ptr<Connection>> connections_;
    std::shared_ptr<Epoll> epoller_;
    std::shared_ptr<TimerManager> tm_;
    struct epoll_event recvs[max_fd];
    func_t OnMessage_;
    task_t TaskPush_;
};



#endif