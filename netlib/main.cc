#include "net/EventLoop/EventLoop.h"
#include "net/TcpServer/TcpServer.h"

int main()
{
    EventLoop loop;
    // 重点：强制转为 uint16_t
    TcpServer server(loop, "0.0.0.0", static_cast<uint16_t>(8888));

    server.start();
    loop.loop();

    return 0;
}