#include "head.h"
using namespace std;

int main() {
    int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(8023);
    server_sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(server_sockfd, (struct sockaddr*)&server_sockaddr, sizeof(server_sockaddr)) == -1) {
        perror("bind error");
        exit(1);
    }

    if (listen(server_sockfd, 20) == -1) {
        perror("listen error");
        exit(1);
    }

    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);

    int connect = accept(server_sockfd, (struct sockaddr*)&client_addr, &length);
    if (connect < 0) {
        perror("connect error");
        exit(1);
    }
    cout << "客户端成功链接" << endl;

    char buffer[1000];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int len = recv(connect, buffer, sizeof(buffer), 0);
        if (strcmp(buffer, "exit") == 0 || len <= 0) break;
        cout << "收到客户端信息：" << buffer << endl;
    }

    close(connect);
    close(server_sockfd);
    return 0;
}