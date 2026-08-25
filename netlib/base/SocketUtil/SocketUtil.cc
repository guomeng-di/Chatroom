#include "SocketUtil.h"
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include "../Logger/Logger.h"
#include <iostream>
#include <mutex>
#include <array>

using namespace std;

bool SocketUtil::sendAll(int fd,const string& data)
{
    static mutex mutex_;

    lock_guard<mutex> lock(mutex_);


    size_t total=0;

    while(total<data.size())
    {
        ssize_t n=send(
            fd,
            data.data()+total,
            data.size()-total,
            MSG_NOSIGNAL
        );


        if(n<0)
        {
            if(errno==EINTR)
                continue;

            return false;
        }


        if(n==0)
            return false;


        total+=n;
    }

    return true;
}