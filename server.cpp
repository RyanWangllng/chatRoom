#include "head.h"
using namespace std;

int main() {
    // 创建服务器套接字，是一个文件描述符
    int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // 定义服务器socket地址结构体
    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;                       // TCP/IPv4协议族
    server_sockaddr.sin_port = htons(8023);                     // 转换端口：主机字节序 -> 网络字节序
    server_sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");   // ip地址，环回地址，相当于本机ip

    // bind将套接字描述符和本地ip地址及端口号绑定
    if (bind(server_sockfd, (struct sockaddr*)&server_sockaddr, sizeof(server_sockaddr)) == -1) {
        perror("bind error");
        exit(1);
    }

    // 监听一个套接字上的连接
    if (listen(server_sockfd, 20) == -1) {
        perror("listen error");
        exit(1);
    }

    // 定义客户端socket地址结构体
    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);

    // 服务器接受客户端连接，connect成功后是用于通信的文件描述符
    int connect = accept(server_sockfd, (struct sockaddr*)&client_addr, &length);
    if (connect < 0) {
        perror("connect error");
        exit(1);
    }
    cout << "客户端成功连接" << endl;

    // 接受缓冲区
    char buffer[1000];

    // 不断接受客户端发来的数据
    while (1) {
        // 清空缓冲区
        memset(buffer, 0, sizeof(buffer));
        // 接收数据，返回接受到数据的长度
        int len = recv(connect, buffer, sizeof(buffer), 0);
        // 客户端发送exit或异常结束时，退出
        if (strcmp(buffer, "exit") == 0 || len <= 0) break;
        cout << "收到客户端信息：" << buffer << endl;
    }

    // 关闭文件描述符
    close(connect);
    close(server_sockfd);
    return 0;
}