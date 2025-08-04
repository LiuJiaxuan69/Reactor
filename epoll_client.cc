#include <iostream>
#include <cstring>
#include <unistd.h>
#include <thread>
#include "tcp.hpp"
using namespace std;

/**
 * 连接到服务器
 * @param sockfd 客户端socket文件描述符
 * @param server 服务器地址结构体
 * @return 成功返回0，失败返回-1
 */
int Connect(int sockfd, const struct sockaddr_in &server)
{
    int cnt = 5;  // 重试次数
    while (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        // 连接失败，尝试重连
        if (cnt == 0)  // 重试次数用完
        {
            printf("Sorry, I am unable to connect to the designated server\n");
            return -1;
        }
        --cnt;
        printf("Try to reconnect..., %d chances remaining\n", cnt);
        sleep(2);  // 等待2秒后重试
    }
    return 0;
}

/**
 * 消息处理线程函数
 * @param sockfd 已连接的socket文件描述符
 * @param server_ip 服务器IP地址
 * @param server_port 服务器端口号
 */
void Handler(const int sockfd, const std::string server_ip, const u_int16_t server_port)
{
    while (true)
    {
        char buffer[1024];  // 接收缓冲区
        struct sockaddr_in temp;
        socklen_t len = sizeof(temp);
        
        // 接收服务器消息
        ssize_t m = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (m > 0)  // 成功接收到数据
        {
            buffer[m] = 0;  // 添加字符串结束符
            cout << "server echo# " << buffer << endl;  // 打印服务器回显
        }
        else if (m == 0)  // 服务器关闭连接
        {
            printf("server [%s: %d] quit\n", server_ip.c_str(), server_port);
            
            // 重新初始化服务器地址结构
            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(server_port);
            inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr); 
            
            sleep(1);  // 等待1秒后尝试重连
            if(Connect(sockfd, server_addr) == 0) continue;  // 重连成功则继续
            exit(1);  // 重连失败则退出
        }
        else  // 接收出错
        {
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    // 参数检查
    if (argc != 3)
    {
        cerr << "Usage: " << argv[0] << " server_ip server_port" << endl;
        return 1;
    }
    
    // 解析命令行参数
    string server_ip = argv[1];
    uint16_t server_port = stoi(argv[2]);
    
    // 初始化服务器地址结构
    struct sockaddr_in server;
    bzero(&server, sizeof(server));  // 清空结构体
    server.sin_port = htons(server_port);  // 设置端口号(网络字节序)
    server.sin_family = AF_INET;  // IPv4地址族
    server.sin_addr.s_addr = inet_addr(server_ip.c_str());  // 设置IP地址
    
    // 创建并连接socket
    Sock sock;
    sock.Socket();  // 创建socket
    sock.Connect(server);  // 连接服务器
    
    // 创建接收线程
    thread receiver(Handler, sock.GetSockfd(), server_ip, server_port);
    
    // 主线程处理用户输入
    while (true)
    {
        string inbuffer;
        usleep(100);  // 短暂休眠，避免CPU占用过高
        cout << "Please Enter# ";  // 提示用户输入
        getline(cin, inbuffer);  // 读取用户输入
        
        // 发送消息到服务器
        ssize_t n = send(sock.GetSockfd(), inbuffer.c_str(), inbuffer.size(), 0);
        if (n == -1)  // 发送失败
        {
            perror("send");
        }
    }
    
    receiver.join();  // 等待接收线程结束(实际上不会执行到这里)
    return 0;
}