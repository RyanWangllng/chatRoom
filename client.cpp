#include "head.h"
using namespace std;

int main() {
    int sock_client = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in seraddr;
    memset(&seraddr, 0, sizeof(seraddr));
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(8023);
    seraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock_client, (struct sockaddr*)&seraddr, sizeof(seraddr)) < 0) {
        perror("connect error");
        exit(1);
    }
    cout << "链接服务器成功！" << endl;

    char sendbuf[100];
    char recvbuf[100];
    while (1) {
        memset(sendbuf, 0, sizeof(sendbuf));
        cin >> sendbuf;
        send(sock_client, sendbuf, strlen(sendbuf), 0);
        if (strcmp(sendbuf, "exit") == 0) break;
    }
    
    close(sock_client);
    return 0;
}