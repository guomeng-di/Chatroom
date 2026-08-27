#pragma once
#include <string>
#include <atomic>
extern std::atomic<bool> connectionAlive;
class MainMenu{

public:
    
    static void run(int fd);

};
