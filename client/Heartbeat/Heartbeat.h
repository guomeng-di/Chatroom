#ifndef HEARTBEAT_H
#define HEARTBEAT_H
#include <cstdint>
class Heartbeat{
public:
    static bool start();
    static void stop();
    static void check(int fd);
    static int getTimerFd();
private:
    static int timerFd_;
};
#endif