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
    close(sock_connection);
    return;
}

void client::SendMsg(int connection) {
    // cout << "发送线程套接字：" << connection << endl;
    // char sendbuf[100];
    while (1) {
        // memset(sendbuf, 0, sizeof(sendbuf));
        // cin >> sendbuf;
        // int ret = send(connection, sendbuf, strlen(sendbuf), 0);
        // if (strcmp(sendbuf, "exit") == 0 || ret <= 0) break;
        string str;
        cin >> str;
        if (connection > 0) {
            // 私聊
            str = "content:" + str;
        } else if (connection < 0) {
            // 群聊
            str = "gr_message:" + str;
        }
        
        // 发送数据
        int ret = send(abs(connection), str.c_str(), str.size(), 0);
        if (str == "content:exit" || str == "gr_message:exit" || ret <= 0) {
            cout << "发送线程准备结束" << endl;
            break;
        }
    }
}

void client::RecvMsg(int connection) {
    // 接受缓冲区
    char recvbuf[1000];
    // 不断接收数据
    while (1) {
        memset(recvbuf, 0, sizeof(recvbuf));
        int len = recv(connection, recvbuf, sizeof(recvbuf), 0);
        string recv_str = static_cast<string>(recvbuf);
        recv_str = recv_str.substr(len - 4);
        // cout << "接受线程收到了啥： " << recv_str << endl;
        if (len <= 0 || recv_str == "exit") {
            cout << "接受线程准备结束" << endl;
            break;
        }
        cout << recvbuf << endl;
        // cout << "收到服务器发来的消息： " << recvbuf << endl;
    }
}

// 客户端处理与用户交互事务
void client::HandleClient(int connection) {
    int choice;
    string name, password, password1;
    
    bool if_login = false;  // 记录是否登录成功
    string login_name;      // 记录登录成功的用户名

    // 发送本地cookie，并接收服务器答复，若答复通过就不登录
    // 先检查是否存在cookie文件
    ifstream f("cookie.txt");
    string cookie_str;
    if (f.good()) {
        f >> cookie_str;
        f.close();
        cookie_str = "cookie:" + cookie_str;
        // 将cookie发送到服务端
        send(connection, cookie_str.c_str(), cookie_str.size(), 0);
        // 接受服务器答复
        char cookie_ret[100];
        memset(cookie_ret, 0, sizeof(cookie_ret));
        recv(connection, cookie_ret, sizeof(cookie_ret), 0);
        // 判断服务器答复是否通过
        string ret_str(cookie_ret);
        if (ret_str != "NULL") {
            // Redis查询到了cookie，通过
            if_login = true;
            login_name = ret_str;
        }
    }

    cout << " ------------------ " << endl;
    cout << "|                  |" << endl;
    cout << "| 请输入您要的选项:|" << endl;
    cout << "|    0:退出        |" << endl;
    cout << "|    1:登录        |" << endl;
    cout << "|    2:注册        |" << endl;
    cout << "|                  |" << endl;
    cout << " ------------------ " << endl;

    while (1) {
        if (if_login) break;

        cin >> choice;
        if (choice == 0) {
            break;

        } else if (choice == 2) { 
            // 注册
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

        } else if (choice == 1 && !if_login) {
            // 登录
            while (1) {
                cout << "用户名：";
                cin >> name;
                cout << "密码：";
                cin >> password;
                string str = "login" + name;
                str += "pass:";
                str += password;

                // 将登录信息发送给服务器
                send(connection, str.c_str(), str.size(), 0);
                char buffer[1000];
                memset(buffer, 0, sizeof(buffer));
                // 接受服务器响应
                recv(connection, buffer, sizeof(buffer), 0);
                string recv_str(buffer);
                
                if (recv_str.substr(0, 2) == "ok") {
                    // 登录成功
                    if_login = true;
                    login_name = name;

                    // 本地建立cookie文件保存sessionid
                    string tmpstr = recv_str.substr(2);
                    tmpstr = "cat > cookie.txt << end \n" + tmpstr + "\nend";
                    system(tmpstr.c_str());
                    
                    cout << "登陆成功！" << endl;
                    break;
                } else {
                    // 登录失败
                    cout << "用户名或密码错误！" << endl;
                }
            }
        }
    }

    // 登录成功
    while (if_login && 1) {
        if (if_login) {
        // 清空终端
        system("clear");
        cout << "               欢迎回来," << login_name << endl;
        cout << " -------------------------------------------" << endl;
        cout << "|                                           |" << endl;
        cout << "|          请选择你要的选项：               |" << endl;
        cout << "|              0:退出                       |" << endl;
        cout << "|              1:发起单独聊天               |" << endl;
        cout << "|              2:发起群聊                   |" << endl;
        cout << "|                                           |" << endl;
        cout << " ------------------------------------------- " << endl;
        }

        cin >> choice;
        if (choice == 0) {
            break;
        } else if (choice == 1) {
            // 私聊
            cout << "请输入对方的用户名：";
            string target_name, content;
            cin >> target_name;

            string sendStr("target:" + target_name + "from:" + login_name);     // 标识目标用户和源用户
            send(connection, sendStr.c_str(), sendStr.size(), 0);               // 向服务器发送目标用户和源用户
            cout << "请输入你想说的话（输入exit退出）：" << endl;

            // 创建发送和接收线程
            thread send_t(client::SendMsg, connection), recv_t(client::RecvMsg, connection);
            send_t.join();
            recv_t.join();
        } else if (choice == 2) {
            // 群聊
            cout << "请输入群号：" << endl;
            int num;
            cin >> num;

            string sendStr("group:" + to_string(num));
            send(connection, sendStr.c_str(), sendStr.size(), 0);
            cout << "请输入你想说的话（输入exit退出）：" << endl;

            // 创建发送和接收线程。发送线程套接字描述符是负数，与私聊区分开
            thread send_t(client::SendMsg, -connection), recv_t(client::RecvMsg, connection);
            send_t.join();
            recv_t.join();
        }
    }
}