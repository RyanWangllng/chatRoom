#include "head.h"
using namespace std;

int main() {
    // 创建客户端套接字，也是一个文件描述符
    int sock_client = socket(AF_INET, SOCK_STREAM, 0);

    // 定义服务器socket地址结构体
    struct sockaddr_in seraddr;
    // memset(&seraddr, 0, sizeof(seraddr));
    seraddr.sin_family = AF_INET;                       // TCP/IPv4协议族
    seraddr.sin_port = htons(8023);                     // 转换端口：主机字节序 -> 网络字节序
    seraddr.sin_addr.s_addr = inet_addr("127.0.0.1");   // ip地址，环回地址，相当于本机ip

    // 客户端请求连接服务器
    if (connect(sock_client, (struct sockaddr*)&seraddr, sizeof(seraddr)) < 0) {
        perror("connect error");
        exit(1);
    }
    cout << "连接服务器成功！" << endl;

    char sendbuf[100];
    char recvbuf[100];
    while (1) {
        memset(sendbuf, 0, sizeof(sendbuf));
        cin >> sendbuf;
        // 在TCP连接上发送数据
        send(sock_client, sendbuf, strlen(sendbuf), 0);
        if (strcmp(sendbuf, "exit") == 0) break;
    }

    // 关闭文件描述符
    close(sock_client);
    return 0;
}