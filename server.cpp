// #include "head.h"
#include "server.h"

// 初始化，sock_arr[i] = false表示套接字描述符i未打开
vector<bool> server::sock_arr(10000, false);
unordered_map<string, int> server::name_sock_map;
pthread_mutex_t server::name_sock_mutex;

// 构造函数
server::server(int port, string ip) : server_port(port), server_ip(ip) {
    // 创建互斥锁
    pthread_mutex_init(&name_sock_mutex, NULL);
}

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
    // 元组类型，四个成员分别是if_login, login_name, target_name, target_conn
    /*
        bool if_login;      //记录当前服务对象是否成功登录
        string login_name;  //记录当前服务对象的名字
        string target_name; //记录目标对象的名字
        int target_conn;    //目标对象的套接字描述符
    */
    tuple<bool, string, string, int> info;
    get<0>(info) = false;
    get<3>(info) = -1;

    // 接收缓冲区
    char buffer[1000];
    // 不断接收数据
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int len = recv(connection, buffer, sizeof(buffer), 0);
        // 客户端发送exit或异常结束时，退出
        if (strcmp(buffer, "content:exit") == 0 || len <= 0) {
            // 关闭套接字描述符
            close(connection);
            sock_arr[connection] = false;
            break;
        }
        cout << "收到套接字描述符为 " << connection << " 的客户端发来的信息： " << buffer << endl;
        
        // 处理客户端请求
        string str(buffer);
        HandleRequest(connection, str, info);

        /*
        // 回复客户端
        string ans = "copy！";
        int ret = send(connection, ans.c_str(), ans.size(), 0);
        // 服务器收到exit或异常关闭套接字描述符
        if (ret <= 0) {
            close(connection);
            sock_arr[connection] = false;
            break;
        }
        */
    }
}

void server::HandleRequest(int connection, string str, tuple<bool, string, string, int>& info) {
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

    // 连接MySQL数据库
    MYSQL mysql_conn;
    mysql_init(&mysql_conn);
    // MYSQL *mysql_conn = mysql_init(NULL);
    // 连接本地数据库，第二个参数使用"127.0.0.1"会连接失败，使用"localhost"连接成功
    mysql_real_connect(&mysql_conn, "localhost", "root", "", "CharProject", 0, NULL, CLIENT_MULTI_STATEMENTS);
    // if (!mysql_real_connect(&mysql_conn, "localhost", "root", "", "CharProject", 0, NULL, CLIENT_MULTI_STATEMENTS)) {
    //     cout << "数据库连接失败！" << endl;
    // } else {
    //     cout << "数据库连接成功！" << endl;
    // }

    if (str.find("name:") != str.npos) {
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
        // int ret = mysql_query(&mysql_conn, search.c_str());
        // if (ret == 0) {
        //     cout << "插入成功！" << endl;
        // } else {
        //     cout << "插入失败！" << ret << endl;
        // }
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

                send(connection, str_ret.c_str(), str_ret.size(), 0);
            } else {
                cout << "登录密码错误！" << endl;
                char str_ret[100] = "wrong";
                send(connection, str_ret, strlen(str_ret), 0);
            }
        } else {
            cout << "查询失败！" << endl;
            char str_ret[100] = "wrong";
            send(connection, str_ret, strlen(str_ret), 0);
        }

        // 释放mysql_store_result()为结果集分配的内存
        mysql_free_result(result);
    } else if (str.find("target:") != str.npos) {
        // 绑定目标用户的文件描述符
        int p1 = str.find("from:");
        string target = str.substr(7, p1 - 7);  // 目标用户名
        string from = str.substr(p1 + 5);       // 源用户名
        target_name = target;

        if (name_sock_map.find(target) == name_sock_map.end()) {
            // 找不到目标用户
            cout << "源用户为 " << login_name << "，向目标用户 " << target_name << " 仍未登录，无法发起私聊！" << endl;
        } else {
            // 找到了目标用户
            cout << "源用户 " << login_name << " 向目标用户 " << target_name << " 发起的私聊即将建立！" << endl;
            target_conn = name_sock_map[target];
            cout << "目标用户的套接字描述符为 " << target_conn << endl;
        }
    } else if (str.find("content:") != str.npos) {
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
    }

    // 更新实参
    get<0>(info) = if_login;
    get<1>(info) = login_name;
    get<2>(info) = target_name;
    get<3>(info) = target_conn;

}