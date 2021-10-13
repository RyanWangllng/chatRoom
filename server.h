#ifndef _SERVER_
#define _SERVER_

#include "head.h"

class server {
    private:
        int server_port;        // 服务器端口号
        int server_sockfd;      // 监听状态的套接字描述符
        string server_ip;       // 服务器ip
        static vector<bool> sock_arr;                       // 修改为静态成员变量，类型变成Vvector<bool>
        static unordered_map<string, int> name_sock_map;    // 名字和套接字描述符映射关系
        static pthread_mutex_t name_sock_mutex;             // 互斥锁，锁住需要修改的name_sock_map的临界区
        static unordered_map<int, set<int>> group_map;      // 记录群号和同一群号下的套接字描述符集合
        static pthread_mutex_t group_mutex;                 // 互斥锁，锁住需要修改的group_map的临界区
        static unordered_map<string, string> from_to_map;   // 记录用户xx要向用户yy发送的信息
        static pthread_mutex_t from_mutex;                  // 互斥锁，锁住需要修改from_to_map的临界区

    public:
        server(int oprt, string ip);            // 构造函数
        ~server();                              // 析构函数
        void run();                             // 服务器开始运行
        static void RecvMsg(int epollfd, int connection);    // 子线程工作的静态函数
        static void HandleRequest(int epollfd, int connection, string str, tuple<bool, string, string, int, int>& info);
        static void setnonblocking(int connection);     // 将套接字设为非阻塞
};

#endif
