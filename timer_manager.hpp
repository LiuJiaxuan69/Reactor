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
            if (timers.top()->cnt >= 5)
            {
                auto timer = timers.top();
                DeleteTop();
                timer->expired_time = std::time(nullptr) + alive_gap;
                timer->cnt = 0;
                timers.emplace(timer);
                timer_map[sockfd] = timer;
            } else if(timers.top()->cnt != timer_map[sockfd]->cnt || timers.top()->expired_time != timer_map[sockfd]->expired_time)
            {
                timers.pop();
                timers.emplace(timer_map[sockfd]);
            }
            else
            {
                return true;
            }
        }
        return false;
    }
    void DeleteTop()
    {
        timer_map.erase(timers.top()->connect->Sockfd());
        timers.pop();
    }
    std::shared_ptr<Timer> GetTop()
    {
        return timers.top();
    }
    void UpdateTime(int sockfd)
    {
        if(timers.empty()) return;
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