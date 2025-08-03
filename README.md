# Reactor 网络框架

## 项目概述

一个基于多Reactor模式的高性能网络框架，采用C++11实现，主要特点包括：

- 多线程Reactor模型
- 非阻塞I/O + epoll边缘触发
- 定时器管理空闲连接
- 环形队列任务分发
- 松耦合架构设计

## 架构设计

### 核心组件

```
+-------------------+     +-------------------+     +-------------------+
|    Listener       |     |   EventLoop       |     |  TimerManager     |
| (监听线程)        |---->| (工作线程)        |<----| (连接超时管理)    |
+-------------------+     +-------------------+     +-------------------+
        |                       ^
        v                       |
+-------------------+     +-------------------+
|  RingQueue        |     |   Connection      |
| (任务队列)        |     | (连接抽象)        |
+-------------------+     +-------------------+
```

### 线程模型

1. **监听线程**：负责接受新连接，将连接信息放入环形队列
2. **工作线程池**：从队列获取连接，处理I/O事件和业务逻辑
3. **定时器线程**：内置在工作线程中，检查空闲连接

## 核心特性

### 高性能设计
- 边缘触发(EPOLLET)模式
- 非阻塞I/O操作
- 动态EPOLLOUT事件注册
- 无锁环形队列

### 连接管理
```cpp
// 示例：添加新连接
loop->AddConnection(sockfd, EPOLLIN|EPOLLET,
    [](auto conn){ loop->Recv(conn); },  // 读回调
    [](auto conn){ loop->Send(conn); },  // 写回调
    [](auto conn){ loop->Except(conn); } // 异常回调
);
```

### 协议处理
```cpp
// 自定义协议处理器
void OnMessage(std::weak_ptr<Connection> conn) {
    auto connection = conn.lock();
    std::string response = Process(connection->Inbuffer());
    connection->AppendOutBuffer(response);
}
```

## 构建与运行

### 依赖
- Linux系统
- GCC 4.8+ (支持C++11)
- CMake 3.5+

### 构建命令
```bash
mkdir build && cd build
cmake ..
make
```

### 运行示例
```bash
./server [port]  # 默认端口6667
```

## 使用示例

### 自定义业务逻辑
```cpp
// 实现自定义消息处理器
void CustomHandler(std::weak_ptr<Connection> conn) {
    // 业务处理逻辑
}

// 创建工作线程
std::shared_ptr<EventLoop> loop(new EventLoop(queue, CustomHandler));
```

### 配置参数
```cpp
// 调整线程数量
const size_t WORKER_THREADS = 8;  

// 修改连接超时时间
TimerManager tm(15, 5);  // 15秒超时，5次活跃重置
```

## 性能指标

测试环境：
- 2核CPU/4GB内存
- Debian 12
- 千兆网络

预期性能指标：

| 指标            | 乐观估计 | 保守估计 | 影响因素                  |
|----------------|---------|---------|-------------------------|
| 并发连接        | 3,000-5,000 | 1,000-3,000 | 内存限制（每个连接约2-4KB开销）|
| 吞吐量(QPS)     | 15,000-25,000 | 8,000-15,000 | CPU计算能力、业务逻辑复杂度 |
| 平均延迟        | <5ms    | <10ms   | 网络I/O和线程竞争          |

## 扩展指南

### 添加新协议
1. 实现协议解析器
2. 注册到EventLoop的OnMessage回调
3. 处理连接生命周期

### 集成第三方服务
```cpp
void OnMessage(std::weak_ptr<Connection> conn) {
    auto connection = conn.lock();
    // 调用外部服务
    std::string result = CallExternalService(connection->Inbuffer());
    connection->AppendOutBuffer(result);
}
```

该代码仅作为学习和参考使用，实际项目中请根据具体需求进行调整和优化。
