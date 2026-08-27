#include "SocketUtil.h"
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include "../Logger/Logger.h"
#include <iostream>
#include <mutex>
#include <array>
#include <sys/select.h>
using namespace std;

bool SocketUtil::sendAll(int fd,const string& data){
    size_t total=0;
    while(total<data.size()){
        ssize_t n=send(fd,data.data()+total,data.size()-total,MSG_NOSIGNAL);
        if(n>0){
            total+=static_cast<size_t>(n);
            continue;
        }
        if(n==0){
            return false;
        }
        if(errno==EINTR){
            continue;
        }
        if(errno==EAGAIN||errno==EWOULDBLOCK){
            fd_set writefds;
            FD_ZERO(&writefds);
            FD_SET(fd,&writefds);
            int ret=select(fd+1,nullptr,&writefds,nullptr,nullptr);
            if(ret<0){
                if(errno==EINTR){
                    continue;
                }
                return false;
            }
            continue;
        }
        return false;
    }
    return true;
}