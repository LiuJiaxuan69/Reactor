#ifndef _TIMER_HPP_
#define _TIMER_HPP_ 1

#include <queue>
#include <vector>
#include <functional>
#include <memory>
#include <ctime>
#include <iostream>
#include <unordered_map>
#include "connection.hpp"

class Connection;

struct Timer
{
    Timer(int expired_time_, int cnt_, std::shared_ptr<Connection> connect_)
    :expired_time(expired_time_),
    cnt(cnt_),
    connect(connect_){}
    int expired_time;
    int cnt;
    std::shared_ptr<Connection> connect;
};

struct time_cmp
{
    bool operator()(const std::shared_ptr<Timer> t1, const std::shared_ptr<Timer> t2)
    {
        return t1->expired_time > t2->expired_time;
    }
};


#endif