#include <iostream>
#include "protocol.hpp"  // 自定义协议头文件
#include "tcp.hpp"       // 自定义TCP封装头文件

using namespace std;

/**
 * 带重试机制的连接函数
 * @param sockfd 客户端socket文件描述符
 * @param server 服务器地址结构体
 * @return 成功返回0，失败返回-1
 */
int Connect(int sockfd, const struct sockaddr_in &server)
{
    int cnt = 5;  // 最大重试次数
    while (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        // 连接失败处理
        if (cnt == 0)  // 重试次数耗尽
        {
            printf("Sorry, I am unable to connect to the designated server\n");
            return -1;
        }
        --cnt;
        printf("Try to reconnect..., %d chances remaining\n", cnt);
        sleep(2);  // 等待2秒后重试
    }
    return 0;  // 连接成功
}

int main(int argc, char *argv[])
{
    // 参数校验
    if (argc != 3)
    {
        cerr << "\nUsage: " << argv[0] << " server_ip server_port\n"
             << endl;
        return 1;
    }

    // 解析命令行参数
    string server_ip = argv[1];
    uint16_t server_port = stoi(argv[2]);

    // 创建并初始化socket
    Sock sock;
    sock.Socket();  // 创建socket

    // 配置服务器地址结构
    struct sockaddr_in server;
    bzero(&server, sizeof(server));  // 清空结构体
    server.sin_port = htons(server_port);  // 设置端口(网络字节序)
    server.sin_family = AF_INET;  // IPv4地址族
    server.sin_addr.s_addr = inet_addr(server_ip.c_str());  // 设置IP地址

    // 连接服务器
    Connect(sock.GetSockfd(), server);

    // 准备接收缓冲区
    std::string recv_str;
    char buffer[1024];

    // 主循环 - 计算器交互界面
    while (true)
    {
        string x_str, y_str;
        char op;
        int x = 0, y = 0;

        // 输入并验证x的值
        while (true)
        {
            cout << "Please Enter x: ";
            cin >> x_str;
            try
            {
                x = stoi(x_str);  // 尝试转换为整数
                break;  // 转换成功则退出循环
            }
            catch (const exception &e)
            {
                cout << "Invalid number! Please enter an integer." << endl;
            }
        }

        // 输入并验证y的值
        while (true)
        {
            cout << "Please Enter y: ";
            cin >> y_str;
            try
            {
                y = stoi(y_str);  // 尝试转换为整数
                break;  // 转换成功则退出循环
            }
            catch (const exception &e)
            {
                cout << "Invalid number! Please enter an integer." << endl;
            }
        }

        // 输入运算符
        cout << "Please Enter op: ";
        cin >> op;

        // 构造请求对象并序列化
        Request req(x, y, op);
        string content = req.Serialize();  // 序列化为字符串
        content = Encode(content);  // 编码(添加长度头等)

        // 发送请求到服务器
        send(sock.GetSockfd(), content.c_str(), content.size(), 0);

        // 接收服务器响应
        int n = recv(sock.GetSockfd(), buffer, sizeof(buffer) - 1, 0);
        if (n == -1)  // 接收出错
        {
            perror("recv");
            exit(1);
        }
        else if (n == 0)  // 服务器关闭连接
        {
            printf("server [%s: %d] quit\n", server_ip.c_str(), server_port);
            
            // 尝试重新连接
            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(server_port);
            inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr); 
            
            sock.Socket();  // 创建新socket
            sleep(1);  // 等待1秒
            
            if(Connect(sock.GetSockfd(), server_addr) == 0) 
                continue;  // 重连成功则继续
            exit(1);  // 重连失败则退出
        }

        // 处理接收到的数据
        buffer[n] = 0;  // 添加字符串结束符
        recv_str += buffer;  // 追加到接收缓冲区

        // 解析响应
        Response resp;
        while (true)
        {
            std::string content;
            if (!Decode(recv_str, content))  // 解码(处理粘包)
                break;
            
            resp.Deserialize(content);  // 反序列化响应
            cout << "result: " << resp.res_ << ", code: " << resp.code_ << endl;
        }
    }
    return 0;
}