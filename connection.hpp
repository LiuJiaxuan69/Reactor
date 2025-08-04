#ifndef _CONNECTION_HPP_
#define _CONNECTION_HPP_ 1
// 防止头文件重复包含的标准保护措施

#include <iostream>
#include <memory>       // 智能指针支持
#include <functional>   // 函数对象支持
#include "common.hpp"   // 项目通用头文件

class Connection;
// Connection类的前向声明，用于func_t类型定义

using func_t = std::function<void(std::weak_ptr<Connection>)>;
// 定义回调函数类型：
// 参数为Connection的弱指针，无返回值
// 使用weak_ptr避免循环引用

class EventLoop; 
// EventLoop类的前向声明

class Connection{
public:
    // 构造函数：初始化socket描述符，默认不关注写事件
    Connection(int sock)
    :sock_(sock),
    write_care_(false){}

    // 获取socket文件描述符
    int Sockfd(){ return sock_; }

    // 追加数据到输入缓冲区
    void AppendInBuffer(const std::string &info)
    {
        inbuffer_ += info;
    }

    // 追加数据到输出缓冲区
    void AppendOutBuffer(const std::string &info)
    {
        outbuffer_ += info;
    }

    // 获取输入缓冲区引用
    std::string &Inbuffer()
    {
        return inbuffer_;
    }

    // 获取输出缓冲区引用
    std::string &OutBuffer()
    {
        return outbuffer_;
    }
private:
    int sock_;              // 套接字文件描述符
    std::string inbuffer_;  // 输入数据缓冲区
    std::string outbuffer_; // 输出数据缓冲区

public:
    // 所属EventLoop的弱引用（避免循环引用）
    std::weak_ptr<EventLoop> el;

    // 三个核心回调函数：
    func_t recv_cb;    // 读事件回调
    func_t send_cb;    // 写事件回调 
    func_t except_cb;  // 异常事件回调

    // 客户端信息
    std::string ip_;     // 客户端IP地址
    uint16_t port_;      // 客户端端口号
    
    // 写事件关注标志
    // true表示需要监听EPOLLOUT事件
    bool write_care_;  
};
#endif