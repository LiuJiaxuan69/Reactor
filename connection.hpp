#ifndef _CONNECTION_HPP_
#define _CONNECTION_HPP_ 1

#include <iostream>
#include <memory>
#include <functional>
#include "common.hpp"

class Connection;

using func_t = std::function<void(std::weak_ptr<Connection>)>;

class EventLoop;

class Connection{
public:
    Connection(int sock)
    :sock_(sock),
    write_care_(false){}
    int Sockfd(){ return sock_; }
    void AppendInBuffer(const std::string &info)
    {
        inbuffer_ += info;
    }
    void AppendOutBuffer(const std::string &info)
    {
        outbuffer_ += info;
    }
    std::string &Inbuffer()
    {
        return inbuffer_;
    }
    std::string &OutBuffer()
    {
        return outbuffer_;
    }
private:
    int sock_;
    std::string inbuffer_; 
    std::string outbuffer_;
public:
    std::weak_ptr<EventLoop> el;
    func_t recv_cb;
    func_t send_cb;
    func_t except_cb;

    std::string ip_;
    uint16_t port_;
    bool write_care_;
};

#endif