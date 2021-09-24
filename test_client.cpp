#include "client.h"

int main() {
    // 创建客户端
    client client1(8023, "127.0.0.1");
    // 运行客户端
    client1.run();
}