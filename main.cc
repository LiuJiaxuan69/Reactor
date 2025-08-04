#include <iostream>
#include <functional>
#include <memory>
#include <unordered_map>
#include <thread>
#include <vector>
#include "log.hpp"
#include "event_loop.hpp"
#include "ring_queue.hpp"
#include "listener.hpp"
#include "server_cal.hpp"

const size_t default_thread_num = 5;  // 默认工作线程数

ServerCal sc;  // 业务逻辑处理器实例

/**
 * @brief 消息处理回调函数
 * @param wconnectiion 客户端连接弱引用
 * @note 处理客户端输入并返回计算结果
 */
void MessageHandler(std::weak_ptr<Connection> wconnectiion) {
    auto connection = wconnectiion.lock();
    std::string &inf = connection->Inbuffer();  // 获取输入缓冲区
    std::string outinf;
    
    while (true) {
        outinf = sc.Calculator(inf);  // 调用业务逻辑处理
        if (outinf.empty()) return;   // 无输出则结束
        
        connection->AppendOutBuffer(outinf);  // 写入输出缓冲区
        
        // 通过EventLoop发送响应
        auto wsender = connection->el;
        auto sender = wsender.lock();
        sender->Send(connection);
    }
}

/**
 * @brief 任务获取回调函数
 * @param wrq 环形队列弱引用
 * @param wel 事件循环弱引用
 * @note 从队列获取新连接并注册到EventLoop
 */
void TaskPush(std::weak_ptr<RingQueue<ClientInf>> wrq, std::weak_ptr<EventLoop> wel) {
    auto rq = wrq.lock();
    auto el = wel.lock();
    
    if (auto client_inf = rq->Pop()) {  // 从队列获取新连接
        el->AddConnection(
            client_inf->sockfd, 
            EPOLLIN | EPOLLET,  // 边缘触发模式
            std::bind(&EventLoop::Recv, el, std::placeholders::_1),  // 读回调
            std::bind(&EventLoop::Send, el, std::placeholders::_1),  // 写回调
            std::bind(&EventLoop::Except, el, std::placeholders::_1), // 异常回调
            client_inf->client_ip, 
            client_inf->client_port
        );
    }
}

/**
 * @brief 监听线程处理函数
 * @param rq 环形队列
 * @param port 监听端口
 */
void ListenHandler(std::shared_ptr<RingQueue<ClientInf>> rq, uint16_t port) {
    std::shared_ptr<Listener> lt(new Listener(port));  // 创建监听器
    lt->Init();  // 初始化监听socket
    
    // 主EventLoop只负责监听
    std::shared_ptr<EventLoop> baser(new EventLoop(rq));
    baser->AddConnection(
        lt->Fd(), 
        EPOLLIN | EPOLLET, 
        std::bind(&Listener::Accepter, lt, std::placeholders::_1),  // 接受新连接
        nullptr,  // 无需写回调
        nullptr,  // 无需异常回调
        "0.0.0.0", 
        0, 
        true  // 标记为监听socket
    );
    baser->Loop();  // 启动事件循环
}

/**
 * @brief 工作线程处理函数
 * @param rq 环形队列
 */
void EventHandler(std::shared_ptr<RingQueue<ClientInf>> rq) {
    // 创建工作EventLoop，设置消息处理和任务获取回调
    std::shared_ptr<EventLoop> task_handler(
        new EventLoop(rq, MessageHandler, TaskPush)
    );
    task_handler->Loop();  // 启动事件循环
}

int main(int argc, char *argv[]) {
    // 参数处理
    uint16_t port;
    if(argc == 1) port = 6667;  // 默认端口
    else if(argc == 2) port = std::stoi(argv[1]);
    else {
        std::cerr << "Usage: " << argv[0] << " [port]" << std::endl;
        return 1;
    }

    // 创建环形队列
    std::shared_ptr<RingQueue<ClientInf>> rq(new RingQueue<ClientInf>);
    
    // 启动监听线程
    std::thread base_thread(ListenHandler, rq, port);
    
    // 创建工作线程池
    std::vector<std::thread> threads;
    for (int i = 0; i < default_thread_num; ++i) {
        threads.emplace_back(std::thread(EventHandler, rq));
    }
    
    // 等待所有线程结束
    for (auto &thread : threads) {
        if (thread.joinable()) thread.join();
    }
    base_thread.join();

    return 0;
}