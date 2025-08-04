#ifndef _EPOLL_HPP_
#define _EPOLL_HPP_ 1

#include <iostream>
#include <sys/epoll.h>  // epoll系统调用头文件
#include "nocopy.hpp"   // 禁止拷贝的基类
#include "log.hpp"      // 日志系统头文件

// 默认epoll_wait超时时间(毫秒)
inline const int default_time_out = 3000;

// 错误码枚举
enum {
    epoll_create_error = 1,  // epoll创建失败错误码
};

/**
 * @brief Epoll封装类，继承nocopy禁止拷贝构造
 */
class Epoll: public nocopy
{
public:
    /**
     * @brief 构造函数：创建epoll实例
     */
    Epoll()
    :timeout(default_time_out)  // 初始化超时时间
    {
        epfd = epoll_create1(0);  // 创建epoll实例
        if(epfd == -1)
        {
            // 记录创建失败日志（线程ID、错误码、错误信息）
            lg(Error, "thread-%d, epoll_create false, errno: %d, errstr: %s", 
               pthread_self(), errno, strerror(errno));
            exit(epoll_create_error);  // 创建失败直接退出
        }
        // 记录创建成功日志
        lg(Info, "thread-%d, epoll create success, epoll fd: %d", pthread_self(), epfd);
    }

    /**
     * @brief 等待epoll事件
     * @param events 输出参数，用于接收事件数组
     * @param num 最大事件数量
     * @return 就绪的事件数量，-1表示错误
     */
    int EpollWait(struct epoll_event events[], int num)
    {
        int n = epoll_wait(epfd, events, num, timeout);
        if(n == -1){
            lg(Error, "epoll_wait false, errno: %d, errstr: %s", errno, strerror(errno));
        }
        return n;
    }

    /**
     * @brief 控制epoll监控的文件描述符
     * @param op 操作类型 (EPOLL_CTL_ADD/MOD/DEL)
     * @param fd 要操作的文件描述符
     * @param event 要监控的事件标志
     */
    void EpollCtl(int op, int fd, uint32_t event)
    {
        if(op == EPOLL_CTL_DEL){
            // 删除操作不需要event参数
            if(epoll_ctl(epfd, op, fd, nullptr) == -1){
                lg(Error, "epoll control false, errno: %d, errstr: %s", 
                   errno, strerror(errno));
            }
        }else{
            struct epoll_event ev;
            ev.data.fd = fd;    // 关联文件描述符
            ev.events = event;    // 设置监听事件
            if(epoll_ctl(epfd, op, fd, &ev)){
                lg(Error, "epoll control false, errno: %d, errstr: %s", 
                   errno, strerror(errno));
            }
        }
    }

    /**
     * @brief 析构函数：关闭epoll文件描述符
     */
    ~Epoll()
    {
        close(epfd);  // 确保资源释放
    }

private:
    int epfd;      // epoll文件描述符
    int timeout;   // epoll_wait超时时间(毫秒)
};

#endif