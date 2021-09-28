#include "client.h"

client::client(int port, string ip) : server_port(port), server_ip(ip) {}
client::~client() {
    close(sock_connection);
}

void client::run() {
    sock_connection = socket(AF_INET, SOCK_STREAM, 0);

    // 定义服务器socket地址结构体
    struct sockaddr_in servaddr;
    // memset(&seraddr, 0, sizeof(seraddr));
    servaddr.sin_family = AF_INET;                               // TCP/IPv4协议族
    servaddr.sin_port = htons(server_port);                      // 转换端口：主机字节序 -> 网络字节序
    servaddr.sin_addr.s_addr = inet_addr(server_ip.c_str());     // ip地址，环回地址，相当于本机ip

    // 客户端请求连接服务器
    if (connect(sock_connection, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect error");
        exit(1);
    }
    cout << "连接服务器成功！" << endl;

    HandleClient(sock_connection);

    /*
    // 创建发送线程和接收线程
    thread send_t(client::SendMsg, sock_connection), recv_t(client::RecvMsg, sock_connection);
    send_t.join();
    cout << "发送线程已关闭！" << endl;
    recv_t.join();
    cout << "接收线程已关闭！" << endl;
    */
    return;
}

void client::SendMsg(int connection) {
    char sendbuf[100];
    while (1) {
        memset(sendbuf, 0, sizeof(sendbuf));
        cin >> sendbuf;
        // 发送数据
        int ret = send(connection, sendbuf, strlen(sendbuf), 0);
        if (strcmp(sendbuf, "exit") == 0 || ret <= 0) break;
    }
}

void client::RecvMsg(int connection) {
    // 接受缓冲区
    char recvbuf[1000];
    // 不断接收数据
    while (1) {
        memset(recvbuf, 0, sizeof(recvbuf));
        int len = recv(connection, recvbuf, sizeof(recvbuf), 0);
        if (len <= 0) break;
        cout << "收到服务器发来的消息： " << recvbuf << endl;
    }
}

// 客户端处理与用户交互事务
void client::HandleClient(int connection) {
    int choice;
    string name, password, password1;

    cout << " ------------------ " << endl;
    cout << "|                  |" << endl;
    cout << "| 请输入您要的选项:|" << endl;
    cout << "|    0:退出        |" << endl;
    cout << "|    1:登录        |" << endl;
    cout << "|    2:注册        |" << endl;
    cout << "|                  |" << endl;
    cout << " ------------------ " << endl;

    while (1) {
        cin >> choice;
        if (choice == 0) {
            break;
        } else if (choice == 2) {
            cout << "注册用户名：";
            cin >> name;
            while (1) {
                cout << "密码：";
                cin >> password;
                cout << "确认密码：";
                cin >> password1;
                if (password1 == password) {
                    break;
                } else {
                    cout << "密码不一致，请重新输入！" << endl;
                }
            }
            name = "name:" + name;
            password = "password:" + password;
            string str = name + password;
            send(connection, str.c_str(), str.size(), 0);
            cout << "注册成功!" << endl;
            cout << "继续输入您要的选项：" << endl;
        }
    }
}