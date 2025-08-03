#ifndef _RING_QUEUE_HPP_
#define _RING_QUEUE_HPP_ 1

#include <iostream>
#include <pthread.h>
#include <vector>
#include <semaphore.h>
#include <optional>

class LockGuard
{
public:
    explicit LockGuard(pthread_mutex_t &mtx)
    :mtx_(mtx)
    {
        pthread_mutex_lock(&mtx_);
    }
    ~LockGuard()
    {
        pthread_mutex_unlock(&mtx_);
    }

    LockGuard(const LockGuard&) = delete;            // 拷贝构造禁用
    LockGuard& operator=(const LockGuard&) = delete; // 赋值操作禁用

private:
    pthread_mutex_t &mtx_;
};

template<class T>
class RingQueue
{
private:
    int P(sem_t &sem)
    {
        return sem_trywait(&sem);
    }
    void V(sem_t &sem)
    {
        sem_post(&sem);
    }
public:
    RingQueue(size_t cap = 10)
    :cap_(cap),
    c_index_(0),
    p_index_(0),
    queue(cap)
    {
        pthread_mutex_init(&p_mutex_, nullptr);
        pthread_mutex_init(&c_mutex_, nullptr);
        sem_init(&space_sem_, 0, cap_);
        sem_init(&data_sem_, 0, 0);
    }
    ~RingQueue()
    {
        pthread_mutex_destroy(&p_mutex_);
        pthread_mutex_destroy(&c_mutex_);
        sem_destroy(&space_sem_);
        sem_destroy(&data_sem_);
    }
    void Push(const T & task)
    {
        if(P(space_sem_) == -1){
            return;
        }
        {
            LockGuard lg(p_mutex_);
            queue[p_index_] = task;
            p_index_ = (p_index_ + 1) % cap_;
        }
        V(data_sem_);
    }
    std::optional<T> Pop()
    {
        if(P(data_sem_) == -1){
            return std::nullopt;
        }
        T task;
        {
            LockGuard lg(c_mutex_);
            task = queue[c_index_];
            c_index_ = (c_index_ + 1) % cap_;
        }
        V(space_sem_);
        return task;
    }
private:
    sem_t space_sem_;
    sem_t data_sem_;
    pthread_mutex_t p_mutex_;
    pthread_mutex_t c_mutex_;
    size_t cap_;
    size_t c_index_;
    size_t p_index_;
    std::vector<T> queue;
};

#endif
