#ifndef _TIMER_MANAGER_HPP_
#define _TIMER_MANAGER_HPP_ 1

#include "timer.hpp"

inline const int default_alive_gap = 10;
inline const int default_alive_cnt = 5;


class TimerManager
{
public:
    TimerManager(int gap = default_alive_gap, int cnt = default_alive_cnt)
        : alive_gap(gap),
          alive_cnt(cnt) {}
    void Push(std::shared_ptr<Connection> connect)
    {
        std::shared_ptr<Timer> timer(new Timer(std::time(nullptr) + alive_gap, 0, connect));
        timers.emplace(timer);
        timer_map[connect->Sockfd()] = timer;
    }
    bool IsTopExpired()
    {
        while (!timers.empty() && timers.top()->expired_time <= std::time(nullptr))
        {
            int sockfd = (timers.top()->connect)->Sockfd();
            // 处理惰性删除
            if(timer_map.find(sockfd) == timer_map.end()) {
                timers.pop();
                continue;
            }
            if(timers.top()->cnt != timer_map[sockfd]->cnt || timers.top()->expired_time != timer_map[sockfd]->expired_time)
            {
                timers.pop();
                timers.emplace(timer_map[sockfd]);
            }
            else if (timers.top()->cnt >= 5)
            {
                auto timer = timers.top();
                DeleteTop();
                timer->expired_time = std::time(nullptr) + alive_gap;
                timer->cnt = 0;
                timers.emplace(timer);
                timer_map[sockfd] = timer;
            }
            else
            {
                return true;
            }
        }
        return false;
    }

    void LazyDelete(int sockfd){
        timer_map.erase(sockfd);
    }
    std::shared_ptr<Timer> GetTop()
    {
        return timers.top();
    }
    void UpdateTime(int sockfd)
    {
        // 检测链接是否已被删除
        if(timer_map.find(sockfd) == timer_map.end()) return;
        if(timers.empty()) return;
        // 创建新的 Timer（避免共享指针副作用）
        auto new_timer = std::make_shared<Timer>(
        std::time(nullptr) + alive_gap,
        timer_map[sockfd]->cnt + 1,
        timer_map[sockfd]->connect
        );

    // 更新 timer_map
        timer_map[sockfd] = new_timer;
        timer_map[sockfd]->cnt++;
        timer_map[sockfd]->expired_time = time(nullptr) + alive_gap;
    }

private:
    std::priority_queue<std::shared_ptr<Timer>, std::vector<std::shared_ptr<Timer>>, time_cmp> timers;
    std::unordered_map<int, std::shared_ptr<Timer>> timer_map;
    int alive_gap;
    int alive_cnt;
};


#endif