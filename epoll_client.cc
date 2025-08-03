#include <iostream>
#include <cstring>
#include <unistd.h>
#include <thread>
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

void Handler(const int sockfd, const std::string server_ip, const u_int16_t server_port)
{
    while (true)
    {
        char buffer[1024];
        struct sockaddr_in temp;
        socklen_t len = sizeof(temp);
        // 这里并不关心接收方信息
        ssize_t m = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (m > 0)
        {
            buffer[m] = 0;
            cout << "server echo# " << buffer << endl;
        }
        else if (m == 0)
        {
            printf("server [%s: %d] quit\n", server_ip.c_str(), server_port);
            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(server_port);
            inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr); 
            sleep(1);
            if(Connect(sockfd, server_addr) == 0) continue;
            exit(1);
        }
        else
        {
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cerr << "Usage: " << argv[0] << " server_ip server_port" << endl;
        return 1;
    }
    string server_ip = argv[1];
    uint16_t server_port = stoi(argv[2]);
    struct sockaddr_in server;
    bzero(&server, sizeof(server));
    server.sin_port = htons(server_port);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(server_ip.c_str());
    Sock sock;
    sock.Socket();
    sock.Connect(server);
    thread receiver(Handler, sock.GetSockfd(), server_ip, server_port);
    while (true)
    {

        string inbuffer;
        usleep(100);
        cout << "Please Enter# ";
        getline(cin, inbuffer);
        // 发送给 server
        ssize_t n = send(sock.GetSockfd(), inbuffer.c_str(), inbuffer.size(), 0);
        if (n == -1)
        {
            perror("send");
        }
    }
    receiver.join();
    return 0;
}