#ifndef _CLIENT_
#define _CLIENT_

#include "head.h"

class client {
    private:
        int server_port;        // 服务器端口
        string server_ip;       // 服务器ip
        int sock_connection;    // 与服务器建立连接的套接字描述符

    public:
        client(int port, string ip);
        ~client();
        void run();                             // 启动客户端
        static void SendMsg(int connection);    // 发送线程        
        static void RecvMsg(int connection);    // 接收线程
};

#endif