// #include "head.h"
#include "server.h"
#include "threadPool.hpp"

// 初始化，sock_arr[i] = false表示套接字描述符i未打开
vector<bool> server::sock_arr(10000, false);
unordered_map<string, int> server::name_sock_map;
pthread_mutex_t server::name_sock_mutex;
unordered_map<int, set<int>> server::group_map;
pthread_mutex_t server::group_mutex;
unordered_map<string, string> server::from_to_map;
pthread_mutex_t server::from_mutex;

// 构造函数
server::server(int port, string ip) : server_port(port), server_ip(ip) {
    // 创建互斥锁
    pthread_mutex_init(&name_sock_mutex, NULL);
    pthread_mutex_init(&group_mutex, NULL);
    pthread_mutex_init(&from_mutex, NULL);
}

// 析构函数
server::~server() {
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
    // 未连接的和已连接的和的最大值
    int LISTEN_BACKLOG = 128;
    ssize_t n;

    // 声明epoll_event结构体的变量，event用于注册事件，数组用于回传要处理的事件
    struct epoll_event event, events[10000];
    
    // 生成用于处理accept的epoll专用的文件描述符
    int epfd = epoll_create(10000);

    // 服务器和客户端的socket地址结构体
    struct sockaddr_in clientaddr;
    struct sockaddr_in serveraddr;

    // 监听文件描述符
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);

    // 设置为非阻塞
    setnonblocking(listenfd);

    // 设置与要处理的事件相关的文件描述符
    event.data.fd = listenfd;

    // 设置要处理的事件类型
    event.events = EPOLLIN | EPOLLET;

    // 注册epoll事件
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &event);

    // 设置serveraddr
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;                        // TCP/IPv4协议族
    serveraddr.sin_port = htons(8023);                      // 转换端口：主机字节序 -> 网络字节序
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");    // ip地址，环回地址，相当于本机ip

    // 绑定
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1) {
        perror("bind error");
        exit(1);
    }

    // 监听
    if (listen(listenfd, LISTEN_BACKLOG) == -1) {
        perror("listen error");
        exit(1);
    }

    socklen_t clilen = sizeof(clientaddr);

    // 定义一个最小3线程，最大10线程的线程池
    ThreadPool<int> pool(3, 10);

    // 不断与客户端创建新的连接，并创建子线程为其服务
    while (1) {
        cout << "------------------------" << endl;
        cout << "epoll_wait 阻塞中..." << endl;

        // 等待epoll事件的发生
        // 最后一个参数时timeout，0：立即返回；-1：一直阻塞直到有时间；x：等待x毫秒
        int ndfs = epoll_wait(epfd, events, 1000, -1);
        cout << "epoll_wait返回，有事件发生！" << endl;

        // 处理所发生的的事件
        for (int i = 0; i < ndfs; ++i) {
            if (events[i].data.fd == listenfd) {
                // 有新客户端连接服务器
                int connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clilen);
                if (connfd < 0) {
                    perror("connfd < 0");
                    exit(1);
                } else {
                    cout << "用户：" << inet_ntoa(clientaddr.sin_addr) << "正在连接..." <<  endl;
                }
                // 设置用于读操作的文件描述符
                event.data.fd = connfd;
                // 设置用于注册的读操作事件，采用ET边缘触发，为防止多个线程处理同一个socket而使用EPOLLONESHOT
                event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                // 边缘触发要将套接字设置为非阻塞
                setnonblocking(connfd);
                // 注册event
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &event);
            
            } else if (events[i].events & EPOLLIN) {
                // 接收到读事件
                int sockfd = events[i].data.fd;
                events[i].data.fd = -1;
                cout << "接受到读事件！" << endl;
                string recv_str;
                // 加入任务队列，处理事件
                pool.addTask(Task<int>(RecvMsg, epfd, sockfd));
            }
        }
    }
    close(listenfd);
}

// 子线程工作的静态函数
// 前面加static会报错
void server::RecvMsg(int epollfd, int connection) {
    // 元组类型，五个成员分别是if_login, login_name, target_name, target_conn, group_num
    /*
        bool if_login;      // 记录当前服务对象是否成功登录
        string login_name;  // 记录当前服务对象的名字
        string target_name; // 记录目标对象的名字
        int target_conn;    // 目标对象的套接字描述符
        int group_num;      // 记录所处的群号
    */
    tuple<bool, string, string, int, int> info;
    get<0>(info) = false;
    get<3>(info) = -1;

    string recv_str;
    // 不断接收数据
    while (1) {
        // 接收缓冲区
        char buffer[100];        
        memset(buffer, 0, sizeof(buffer));
        int len = recv(connection, buffer, sizeof(buffer), 0);
        if (len < 0) {
            cout << "recv返回值小于0!" << endl;
            // 对于非阻塞IO，下面的事件成立表示数据已经全部读取完毕
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                cout << "数据读取完毕！" << endl;
                cout << "接收到的完整内容为：" << recv_str << endl;
                cout << "开始处理事件！" << endl;
                break;
            }
            cout << "errno: " << errno << endl;
            return;

        } else if (len == 0) {
            cout << "recv返回值为0！" << endl;
            return;

        } else {
            cout << "接收到的内容为：" << buffer << endl;
            string tmp(buffer);
            recv_str += tmp;
        }
    }
    HandleRequest(epollfd, connection, recv_str, info);
}

// 将参数的文件描述符设为非阻塞
void server::setnonblocking(int sock) {
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0) {
        perror("fcntl(sock, F_GETFL)");
        exit(1);
    }
    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0) {
        perror("fcntl(sock, F_SETFL, opts)");
        exit(1);
    }
}

void server::HandleRequest(int epollfd, int connection, string str, tuple<bool, string, string, int, int>& info) {
    char buffer[1000];
    string name, password;
    /*
    bool if_login = false;  // 记录当前客户端对象是否登录成功
    string login_name;      // 记录当前客户端对象的用户名
    */
    bool if_login = get<0>(info);       // 记录当前服务对象是否成功登录
    string login_name = get<1>(info);   // 记录当前服务对象的用户名
    string target_name = get<2>(info);  // 记录目标对象的用户名
    int target_conn = get<3>(info);     // 记录目标对象的套接字描述符
    int group_num = get<4>(info);       // 记录所处群号

    // 连接MySQL数据库
    MYSQL mysql_conn;
    mysql_init(&mysql_conn);
    // MYSQL *mysql_conn = mysql_init(NULL);
    // 连接本地数据库，第二个参数使用"127.0.0.1"会连接失败，使用"localhost"连接成功
    mysql_real_connect(&mysql_conn, "localhost", "root", "", "CharProject", 0, NULL, CLIENT_MULTI_STATEMENTS);

    // 连接Redis数据库
    redisContext *redis_target = redisConnect("127.0.0.1", 6379);
    if (redis_target->err) {
        redisFree(redis_target);
        cout << "连接失败！" << endl;
    } else {
        cout << "Redis连接成功！" << endl;
    }

    // 先接收cookie看看Redis是否保存该用户的登陆状态
    if (str.find("cookie:") != str.npos) {
        string cookie = str.substr(7);
        // 查询该cookie是否存在：hget cookie name
        string redis_str = "hget " + cookie + " name";
        redisReply *r = (redisReply*)redisCommand(redis_target, redis_str.c_str());
        string send_res;
        if (r->str) {
            // cookie存在
            cout << "查询Redis结果：" << r->str << endl;
            send_res = r->str;
        } else {
            // cookie不存在
            cout << "cookie not exist！" << endl;
            send_res = "NULL";
        }
        send(connection, send_res.c_str(), send_res.size(), 0);

    } else if (str.find("name:") != str.npos) {
        // 注册
        int p_name = str.find("name:"), p_pass = str.find("password:");
        name = str.substr(p_name + 5, p_pass - 5);
        password = str.substr(p_pass + 9, str.size() - p_pass - 8);

        string search = "INSERT INTO USER VALUES (\"";
        search += name;
        search += "\",\"";
        search += password;
        search += "\");";
        cout << "SQL语句：" << search << endl;
        
        // 执行SQL语句
        mysql_query(&mysql_conn, search.c_str());

    } else if (str.find("login") != str.npos) {
        // 登录
        int p_login = str.find("login"), p_pass = str.find("pass:");
        name = str.substr(p_login + 5, p_pass - 5);
        password = str.substr(p_pass + 5, str.size() - p_pass - 4);
        
        string search = "SELECT * FROM USER WHERE NAME=\"";
        search += name;
        search += "\";";
        cout << "SQL语句：" << search << endl;

        auto search_ret = mysql_query(&mysql_conn, search.c_str());     // 返回0则执行成功
        // 读取查询到的全部结果，存放在MYSQL_RES结构中；读取结果失败或者查询未返回结果集，将返回NULL指针
        auto result = mysql_store_result(&mysql_conn);
        int col = mysql_num_fields(result);     // 获得查询结果集中的列数
        int row = mysql_num_rows(result);       // 获得查询结果集中的行数

        if (search_ret == 0 && row != 0) {
            cout << "查询成功！" << endl;
            auto info = mysql_fetch_row(result);    // 搜索结果集的下一行，返回MYSQL_ROW结构
            cout << "查询到用户名：" << info[0] << " 密码：" << info[1] << endl;

            if (info[1] == password) {
                cout << "登录密码正确！" << endl;
                string str_ret = "ok";
                if_login = true;
                login_name = name;

                pthread_mutex_lock(&name_sock_mutex);   // 上锁
                name_sock_map[login_name] = connection; // 记录用户名和文件描述符的对应关系
                pthread_mutex_unlock(&name_sock_mutex); // 解锁

                // 随机生成sessionid并发送到客户端
                // sessionid大小为10位，每位由数字（0-9）或大写字母或小写字母随机组成
                srand(time(NULL)); // 初始化随机种子
                for (int i = 0; i < 10; i++) {
                    int type = rand() % 3; // type为0代表数字，1代表小写字母，2代表大写字母
                    if (type == 0) {
                        str_ret += '0' + rand() % 9;
                    } else if (type == 1) {
                        str_ret += 'a' + rand() % 26;
                    } else if (type == 2) {
                        str_ret += 'A' + rand() % 26;
                    }
                }
                // 将sessionid存入Redis中
                string redis_str = "hset " + str_ret.substr(2) + " name " + login_name;
                redisReply *r = (redisReply*)redisCommand(redis_target, redis_str.c_str());
                // 设置生存时间，默认300秒
                redis_str = "expire " + str_ret.substr(2) + " 300";
                r = (redisReply*)redisCommand(redis_target, redis_str.c_str());
                cout << "随机生成的sessionid为：" << str_ret.substr(2) << endl;

                send(connection, str_ret.c_str(), str_ret.size(), 0);
            } else {
                // 密码错误
                cout << "登录密码错误！" << endl;
                char str_ret[100] = "wrong";
                send(connection, str_ret, strlen(str_ret), 0);
            }
        } else {
            // 没找到用户名
            cout << "查询失败！" << endl;
            char str_ret[100] = "wrong";
            send(connection, str_ret, strlen(str_ret), 0);
        }

        // 释放mysql_store_result()为结果集分配的内存
        mysql_free_result(result);
        
    } else if (str.find("target:") != str.npos) {
        // 绑定私聊目标用户的文件描述符
        cout << "绑定私聊目标用户！" << endl;
        int p1 = str.find("from:");
        string target = str.substr(7, p1 - 7);  // 目标用户名
        string from = str.substr(p1 + 5);       // 源用户名
        target_name = target;

        if (name_sock_map.find(target) == name_sock_map.end()) {
            // 找不到目标用户
            cout << "源用户为 " << login_name << "，向目标用户 " << target_name << " 仍未登录，无法发起私聊！" << endl;
        } else {
            // 找到了目标用户
            pthread_mutex_lock(&from_mutex);
            from_to_map[from] = target;
            cout << "from: " << from << " target: " << target << endl;
            pthread_mutex_unlock(&from_mutex);
            cout << "源用户 " << login_name << " 向目标用户 " << target_name << " 发起的私聊即将建立！" << endl;
            target_conn = name_sock_map[target];
            cout << "目标用户的套接字描述符为 " << target_conn << endl;
        }

    } else if (str.find("content:") != str.npos) {
        cout << "转发私聊信息" << endl;
        target_conn = -1;
        // 找出目标用户和源用户
        for (auto i : name_sock_map) {
            if (i.second == connection) {
                login_name = i.first;
                target_name = from_to_map[login_name];
                target_conn = name_sock_map[target_name];
                break;
            }
        }
        // 转发私聊信息
        if (target_conn == -1) {
            cout << "找不到目标用户 " << target_name << "，尝试重新寻找目标用户的套接字描述符！" << endl;
            if (name_sock_map.find(target_name) != name_sock_map.end()) {
                target_conn = name_sock_map[target_name];
                cout << "重新寻找目标用户套接字描述符成功！" << endl;
            } else {
                cout << "重新寻找失败，转发失败！" << endl;
            }
        }
        string recv_str(str);
        string send_str = recv_str.substr(8);
        cout << "用户 " << login_name << " 向 " << target_name << " 发送：" << send_str << endl;
        send_str = "[" + login_name + "]: " + send_str;
        send(target_conn, send_str.c_str(), send_str.size(), 0);

    } else if (str.find("group:") != str.npos) {
        cout << "绑定群聊号！" << endl;
        string recv_str(str);
        string num_str = recv_str.substr(6);
        group_num = stoi(num_str);
        for (auto i : name_sock_map) {
            if (i.second == connection) {
                login_name = i.first;
                break;
            }
        }
        cout << "用户 " << login_name << " 绑定群聊号为：" << num_str << endl;
        pthread_mutex_lock(&group_mutex);           // 上锁
        group_map[group_num].insert(connection);    // 将套接字描述符插入该群聊号对应的set中
        pthread_mutex_unlock(&group_mutex);         // 解锁

    } else if (str.find("gr_message:") != str.npos) {
        // 广播群聊消息
        cout << "广播群聊消息！" << endl;
        for (auto i : name_sock_map) {
            if (i.second == connection) {
                login_name = i.first;
                break;
            }
        }
        for (auto i : group_map) {
            if (i.second.find(connection) != i.second.end()) {
                group_num = i.first;
                break;
            }
        }
        string send_str(str);
        send_str = send_str.substr(11);
        send_str = "[" + login_name + "]: " + send_str;
        cout << "群聊消息：" << send_str << endl;
        for (auto i : group_map[group_num]) {
            if (i != connection) {
                send(i, send_str.c_str(), send_str.size(), 0);
            }
        }
    }

    // 线程工作完毕后重新注册事件
    epoll_event event;
    event.data.fd = connection;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, connection, &event);

    mysql_close(&mysql_conn);
    if (!redis_target->err) {
        redisFree(redis_target);
    }

    // 更新实参
    get<0>(info) = if_login;
    get<1>(info) = login_name;
    get<2>(info) = target_name;
    get<3>(info) = target_conn;
    get<4>(info) = group_num;
}