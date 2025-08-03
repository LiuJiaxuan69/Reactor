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

const size_t default_thread_num = 5;

ServerCal sc;

void MessageHandler(std::weak_ptr<Connection> wconnectiion)
{
    auto connection = wconnectiion.lock();
    std::string &inf = connection->Inbuffer();
    std::string outinf;
    while (true)
    {
        outinf = sc.Calculator(inf);
        if (outinf.empty())
            return;
        connection->AppendOutBuffer(outinf);

        auto wsender = connection->el;
        auto sender = wsender.lock();
        sender->Send(connection);
    }
}

void TaskPush(std::weak_ptr<RingQueue<ClientInf>> wrq, std::weak_ptr<EventLoop> wel)
{
    auto rq = wrq.lock();
    auto el = wel.lock();
    if (auto client_inf = rq->Pop())
        el->AddConnection(client_inf->sockfd, EPOLLIN | EPOLLET, std::bind(&EventLoop::Recv, el, std::placeholders::_1),
                          std::bind(&EventLoop::Send, el, std::placeholders::_1),
                          std::bind(&EventLoop::Except, el, std::placeholders::_1),
                          client_inf->client_ip, client_inf->client_port);
}

void ListenHandler(std::shared_ptr<RingQueue<ClientInf>> rq, uint16_t port)
{
    std::shared_ptr<Listener> lt(new Listener(port));
    lt->Init();
    // 主任务，只负责监听
    std::shared_ptr<EventLoop> baser(new EventLoop(rq));
    baser->AddConnection(lt->Fd(), EPOLLIN | EPOLLET, std::bind(&Listener::Accepter, lt, std::placeholders::_1), nullptr, nullptr, "0.0.0.0", 0, true);
    baser->Loop();
}

void EventHandler(std::shared_ptr<RingQueue<ClientInf>> rq)
{
    std::shared_ptr<EventLoop> task_handler(new EventLoop(rq, MessageHandler, TaskPush));
    task_handler->Loop();
}

int main(int argc, char *argv[])
{
    uint16_t port;
    if(argc == 1) port = 6667;
    else if(argc == 2) port = std::stoi(argv[1]);
    else
    {
        std::cerr << "Usage: " << argv[0] << " [port]" << std::endl;
        return 1;
    }
    std::shared_ptr<RingQueue<ClientInf>> rq(new RingQueue<ClientInf>);
    std::thread base_thread(ListenHandler, rq, port);
    std::vector<std::thread> threads;
    for (int i = 0; i < default_thread_num; ++i)
    {
        threads.emplace_back(std::thread(EventHandler, rq));
    }
    for (auto &thread : threads)
    {
        if (thread.joinable())
            thread.join();
    }
    base_thread.join();

    return 0;
}