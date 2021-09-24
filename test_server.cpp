#include "server.h"

int main() {
    // 创建服务器
    server server1(8023, "127.0.0.1");
    // 开启服务器
    server1.run();

    return 0;
}