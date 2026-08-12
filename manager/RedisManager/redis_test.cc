#include "RedisManager.h"
#include <iostream>
using namespace std;

int main()
{
    RedisManager redis;
    redis.connect();

    redis.saveOfflineMessage("tom","{\"from\":\"jack\",\"message\":\"hello\"}");
    cout<<"save success"<<endl;
    return 0;
}