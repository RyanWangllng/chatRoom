#ifndef _SERVER_
#define _SERVER_

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <iostream>
#include <thread>
#include <vector>
using namespace std;

class server {
    private:
        int server_port;        // 服务器端口号
        int server_sockfd;      // 监听状态的套接字描述符
        string server_ip;       // 服务器ip
        vector<int> sock_arr;   // 保存所有的套接字描述符

    public:
        server(int oprt, string ip);            // 构造函数
        ~server();                              // 析构函数
        void run();                             // 服务器开始运行
        static void RecvMsg(int connection);    // 子线程工作的静态函数
};

#endif
