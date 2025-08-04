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

// 前向声明避免循环依赖
class Connection;

/**
 * @brief 定时器节点结构体
 * @note 存储连接的生命周期状态信息
 */
struct Timer
{
    // 构造函数初始化连接状态
    Timer(int expired_time_, int cnt_, std::shared_ptr<Connection> connect_)
    :expired_time(expired_time_),  // 绝对过期时间戳
    cnt(cnt_),                     // 连续活跃次数计数器
    connect(connect_){}            // 管理的连接对象
    
    int expired_time;              // Unix时间戳格式的过期时间
    int cnt;                       // 累计活跃次数（用于奖励机制）
    std::shared_ptr<Connection> connect; // 关联的连接智能指针
};

/**
 * @brief 定时器比较仿函数
 * @note 用于构建最小堆（最早过期时间在堆顶）
 */
struct time_cmp
{
    bool operator()(const std::shared_ptr<Timer> t1, const std::shared_ptr<Timer> t2)
    {
        // 注意：通过大于号实现最小堆
        return t1->expired_time > t2->expired_time;
    }
};

#endif