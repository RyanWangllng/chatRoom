#ifndef _SERVER_
#define _SERVER_

#include "head.h"

class server {
    private:
        int server_port;        // 服务器端口号
        int server_sockfd;      // 监听状态的套接字描述符
        string server_ip;       // 服务器ip
        static vector<bool> sock_arr;   // 修改为静态成员变量，类型变成Vvector<bool>

    public:
        server(int oprt, string ip);            // 构造函数
        ~server();                              // 析构函数
        void run();                             // 服务器开始运行
        static void RecvMsg(int connection);    // 子线程工作的静态函数
};

#endif
