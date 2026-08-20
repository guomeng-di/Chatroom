#include "SocketUtil.h"
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include "../Logger.h"
#include <iostream>

using namespace std;

bool SocketUtil::sendAll(int fd, const string& data){
    size_t total = 0;
    while(total < data.size()){
        ssize_t n = send(fd,data.data() + total,data.size() - total,MSG_NOSIGNAL);
        if(n<0){
            Logger::instance().error("send failed errno="+to_string(errno)+" error="+string(strerror(errno)));
            return false;
        }
        if(n==0){
            Logger::instance().error("send returned 0");
            return false;
        }
        total+=static_cast<size_t>(n);
    }
    return true;
}