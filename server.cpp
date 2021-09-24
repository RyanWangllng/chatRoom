// #include "head.h"
#include "server.h"

// 初始化，sock_arr[i] = false表示套接字描述符i未打开
vector<bool> server::sock_arr(10000, false);

// 构造函数
server::server(int port, string ip) : server_port(port), server_ip(ip) {}

// 析构函数
server::~server() {
    // for (auto connection : sock_arr) {
    //     close(connection);
    // }
    // 调整析构函数
    for (int i = 0; i < sock_arr.size(); i++) {
        if (sock_arr[i]) {
            close(i);
        }
    }
    close(server_sockfd);
}

// 服务器运行
void server::run() {
    // 服务器套接字
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // 服务器的socket地址结构体
    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;                       // TCP/IPv4协议族
    server_sockaddr.sin_port = htons(server_port);                     // 转换端口：主机字节序 -> 网络字节序
    server_sockaddr.sin_addr.s_addr = inet_addr(server_ip.c_str());   // ip地址，环回地址，相当于本机ip

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

    // 不断与客户端创建新的连接，并创建子线程为其服务
    while (1) {
        int connection = accept(server_sockfd, (struct sockaddr*)&client_addr, &length);
        if (connection < 0) {
            perror("connect error");
            exit(1);
        }
        cout << "套接字描述符为 " << connection << " 的客户端成功连接！" << endl;
        sock_arr.push_back(connection);
        // 创建子线程
        thread t(server::RecvMsg, connection);
        // 设置线程分离状态，用join会导致主线程阻塞
        t.detach();
    }
}

// 子线程工作的静态函数
// 前面加static会报错
void server::RecvMsg(int connection) {
    // 接收缓冲区
    char buffer[1000];
    // 不断接收数据
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int len = recv(connection, buffer, sizeof(buffer), 0);
        // 客户端发送exit或异常结束时，退出
        if (strcmp(buffer, "exit") == 0 || len <= 0) {
            // 关闭套接字描述符
            close(connection);
            sock_arr[connection] = false;
            break;
        }
        cout << "收到套接字描述符为 " << connection << " 的客户端发来的信息： " << buffer << endl;
        // 回复客户端
        string ans = "copy！";
        int ret = send(connection, ans.c_str(), ans.size(), 0);
        // 服务器收到exit或异常关闭套接字描述符
        if (ret <= 0) {
            close(connection);
            sock_arr[connection] = false;
            break;
        }
    }
}