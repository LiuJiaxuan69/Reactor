#ifndef _TIMER_MANAGER_HPP_
#define _TIMER_MANAGER_HPP_ 1

#include "timer.hpp"

// 默认配置常量
inline const int default_alive_gap = 10;  // 默认存活周期（秒）
inline const int default_alive_cnt = 5;   // 重置阈值（次）

/**
 * @brief 定时器管理核心类
 * @note 采用最小堆+哈希表双结构实现高效管理
 */
class TimerManager
{
public:
    // 初始化存活周期和计数阈值
    TimerManager(int gap = default_alive_gap, int cnt = default_alive_cnt)
        : alive_gap(gap),
          alive_cnt(cnt) {}

    /**
     * @brief 添加新连接定时器
     * @param connect 要管理的连接对象
     */
    void Push(std::shared_ptr<Connection> connect)
    {
        // 创建新定时器（当前时间+存活周期）
        std::shared_ptr<Timer> timer(new Timer(std::time(nullptr) + alive_gap, 0, connect));
        timers.emplace(timer);                // 插入最小堆
        timer_map[connect->Sockfd()] = timer; // 建立socket到定时器的映射
    }

    /**
     * @brief 检查堆顶是否过期
     * @return true表示存在过期连接需要处理
     */
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

    /// 惰性删除元素
    void LazyDelete(int sockfd){
        timer_map.erase(sockfd);
    }

    /// 获取堆顶定时器（不弹出）
    std::shared_ptr<Timer> GetTop()
    {
        return timers.top();
    }

    /**
     * @brief 更新连接活跃时间
     * @param sockfd 要更新的socket描述符
     */
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
        timer_map[sockfd]->cnt++;       // 增加活跃计数
        timer_map[sockfd]->expired_time = time(nullptr) + alive_gap; // 重置过期时间
    }

private:
    // 数据结构
    std::priority_queue<std::shared_ptr<Timer>, 
                       std::vector<std::shared_ptr<Timer>>, 
                       time_cmp> timers;  // 最小堆
    std::unordered_map<int, std::shared_ptr<Timer>> timer_map;  // 哈希表
    
    // 配置参数
    int alive_gap;  // 存活周期（秒）
    int alive_cnt;  // 重置阈值（次）
};

#endif