#include <iostream>
#include "protocol.hpp"
#include "tcp.hpp"

using namespace std;

int Connect(int sockfd, const struct sockaddr_in &server)
{
    int cnt = 5;
    while (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        // 进来了说明 connect 连接失败，我们尝试重连
        if (cnt == 0)
        {
            printf("Sorry, I am unable to connect to the designated server\n");
            return -1;
        }
        --cnt;
        printf("Try to reconnect..., %d chances remaining\n", cnt);
        sleep(2);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cerr << "\nUsage: " << argv[0] << " server_ip server_port\n"
             << endl;
    }
    std::string serverip = argv[1];
    uint16_t serverport = std::stoi(argv[2]);
    Sock sock;
    sock.Socket();
    string server_ip = argv[1];
    uint16_t server_port = stoi(argv[2]);
    struct sockaddr_in server;
    bzero(&server, sizeof(server));
    server.sin_port = htons(server_port);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(server_ip.c_str());
    Connect(sock.GetSockfd(), server);
    std::string recv_str;
    char buffer[1024];
    while (true)
    {
        string x_str, y_str;
        char op;
        int x = 0, y = 0;
        // 输入x并验证
        while (true)
        {
            cout << "Please Enter x: ";
            cin >> x_str;
            try
            {
                x = stoi(x_str);
                break; // 转换成功则退出循环
            }
            catch (const exception &e)
            {
                cout << "Invalid number! Please enter an integer." << endl;
            }
        }

        // 输入y并验证
        while (true)
        {
            cout << "Please Enter y: ";
            cin >> y_str;
            try
            {
                y = stoi(y_str);
                break; // 转换成功则退出循环
            }
            catch (const exception &e)
            {
                cout << "Invalid number! Please enter an integer." << endl;
            }
        }
        cout << "Please Enter op: ";
        cin >> op;
        Request req(x, y, op);
        string content = req.Serialize();
        content = Encode(content);
        send(sock.GetSockfd(), content.c_str(), content.size(), 0);
        int n = recv(sock.GetSockfd(), buffer, sizeof(buffer) - 1, 0);
        if (n == -1)
        {
            perror("recv");
            exit(1);
        }
        else if (n == 0)
        {
            printf("server [%s: %d] quit\n", server_ip.c_str(), server_port);
            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(server_port);
            inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr); 
            sock.Socket();
            sleep(1);
            if(Connect(sock.GetSockfd(), server_addr) == 0) continue;
            exit(1);
        }
        buffer[n] = 0;
        recv_str += buffer;
        Response resp;
        while (true)
        {
            std::string content;
            if (!Decode(recv_str, content))
                break;
            resp.Deserialize(content);
            cout << "result: " << resp.res_ << ", code: " << resp.code_ << endl;
        }
    }
    return 0;
}