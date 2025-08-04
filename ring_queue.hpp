#ifndef _RING_QUEUE_HPP_
#define _RING_QUEUE_HPP_ 1

#include <iostream>
#include <pthread.h>
#include <vector>
#include <semaphore.h>
#include <optional>

/**
 * RAII风格的互斥锁保护类
 * 构造时加锁，析构时自动解锁
 */
class LockGuard
{
public:
    // 显式构造函数，接收一个互斥锁引用
    explicit LockGuard(pthread_mutex_t &mtx)
    :mtx_(mtx)
    {
        pthread_mutex_lock(&mtx_);  // 构造时加锁
    }
    
    // 析构时自动解锁
    ~LockGuard()
    {
        pthread_mutex_unlock(&mtx_);
    }

    // 禁用拷贝构造和赋值操作
    LockGuard(const LockGuard&) = delete;            
    LockGuard& operator=(const LockGuard&) = delete; 

private:
    pthread_mutex_t &mtx_;  // 引用管理的互斥锁
};

/**
 * 线程安全的环形队列模板类
 * @tparam T 队列元素类型
 */
template<class T>
class RingQueue
{
private:
    // 尝试获取信号量(P操作)
    int P(sem_t &sem)
    {
        return sem_trywait(&sem);  // 非阻塞版本
    }
    
    // 释放信号量(V操作)
    void V(sem_t &sem)
    {
        sem_post(&sem);
    }

public:
    /**
     * 构造函数
     * @param cap 队列容量，默认为10
     */
    RingQueue(size_t cap = 10)
    :cap_(cap),
    c_index_(0),    // 消费者索引初始化为0
    p_index_(0),    // 生产者索引初始化为0
    queue(cap)      // 初始化vector容量
    {
        // 初始化生产者和消费者互斥锁
        pthread_mutex_init(&p_mutex_, nullptr);
        pthread_mutex_init(&c_mutex_, nullptr);
        
        // 初始化空间信号量(初始值为容量大小)
        sem_init(&space_sem_, 0, cap_);
        // 初始化数据信号量(初始值为0)
        sem_init(&data_sem_, 0, 0);
    }
    
    // 析构函数，释放所有资源
    ~RingQueue()
    {
        pthread_mutex_destroy(&p_mutex_);
        pthread_mutex_destroy(&c_mutex_);
        sem_destroy(&space_sem_);
        sem_destroy(&data_sem_);
    }
    
    /**
     * 向队列推送元素(生产者调用)
     * @param task 要添加的元素
     */
    void Push(const T & task)
    {
        // 尝试获取空间信号量(非阻塞)
        if(P(space_sem_) == -1){
            return;  // 获取失败直接返回
        }
        
        {
            // 加生产者锁保护
            LockGuard lg(p_mutex_);
            queue[p_index_] = task;           // 放置元素
            p_index_ = (p_index_ + 1) % cap_; // 更新生产者索引(环形)
        }
        
        // 释放数据信号量(通知消费者有新数据)
        V(data_sem_);
    }
    
    /**
     * 从队列弹出元素(消费者调用)
     * @return 包含元素的optional，队列为空返回nullopt
     */
    std::optional<T> Pop()
    {
        // 尝试获取数据信号量(非阻塞)
        if(P(data_sem_) == -1){
            return std::nullopt;  // 获取失败返回空
        }
        
        T task;
        {
            // 加消费者锁保护
            LockGuard lg(c_mutex_);
            task = queue[c_index_];           // 获取元素
            c_index_ = (c_index_ + 1) % cap_; // 更新消费者索引(环形)
        }
        
        // 释放空间信号量(通知生产者有新空间)
        V(space_sem_);
        return task;  // 返回获取的元素
    }

private:
    sem_t space_sem_;         // 空间信号量(剩余空间计数)
    sem_t data_sem_;          // 数据信号量(可用数据计数)
    pthread_mutex_t p_mutex_; // 生产者互斥锁
    pthread_mutex_t c_mutex_; // 消费者互斥锁
    size_t cap_;             // 队列容量
    size_t c_index_;         // 消费者索引(读取位置)
    size_t p_index_;         // 生产者索引(写入位置)
    std::vector<T> queue;    // 底层存储容器
};

#endif