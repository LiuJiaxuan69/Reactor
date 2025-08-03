#ifndef _EPOLL_HPP_
#define _EPOLL_HPP_ 1

#include <iostream>
#include <sys/epoll.h>
#include "nocopy.hpp"
#include "log.hpp"

inline const int default_time_out = 3000;

enum {
    epoll_create_error = 1,
};

class Epoll: public nocopy
{
public:
    Epoll()
    :timeout(default_time_out)
    {
        epfd = epoll_create1(0);
        if(epfd == -1)
        {
            lg(Error, "thread-%d, epoll_create false, errno: %d, errstr: %s", pthread_self(), errno, strerror(errno));
            exit(epoll_create_error);
        }
        lg(Info, "thread-%d, epoll create success, epoll fd: %d", pthread_self(), epfd);
    }
    int EpollWait(struct epoll_event events[], int num)
    {
        int n = epoll_wait(epfd, events, num, timeout);
        if(n == -1){
            lg(Error, "epoll_wait false, errno: %d, errstr: %s", errno, strerror(errno));
        }
        return n;
    }
    void EpollCtl(int op, int fd, uint32_t event)
    {
        if(op == EPOLL_CTL_DEL){
            if(epoll_ctl(epfd, op, fd, nullptr) == -1){
                lg(Error, "epoll control false, errno: %d, errstr: %s", errno, strerror(errno));
            }
        }else{
            struct epoll_event ev;
            ev.data.fd = fd;
            ev.events = event;
            if(epoll_ctl(epfd, op, fd, &ev)){
                lg(Error, "epoll control false, errno: %d, errstr: %s", errno, strerror(errno));
            }
        }
    }
    ~Epoll()
    {
        close(epfd);
    }
private:
    int epfd;
    int timeout;
};

#endif