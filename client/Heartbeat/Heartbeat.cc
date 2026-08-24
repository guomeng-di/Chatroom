#include "Heartbeat.h"
#include "../../netlib/base/Logger/Logger.h"
#include "../../netlib/base/SocketUtil/SocketUtil.h"
#include "../../protocol/MessageCodec/MessageCodec.h"
#include "../../protocol/MsgId.h"
#include <nlohmann/json.hpp>
#include <sys/timerfd.h>
#include <unistd.h>
#include <cstdint>

using namespace std;
using json=nlohmann::json;

int Heartbeat::timerFd_=-1;

bool Heartbeat::start(){
    if(timerFd_>=0) return true;
    timerFd_=timerfd_create(CLOCK_MONOTONIC,0);
    if(timerFd_<0){
        // Logger::instance().error("heartbeat timerfd_create failed");
        return false;
    }
    itimerspec timer{};
    timer.it_value.tv_sec=5;
    timer.it_interval.tv_sec=5;
    if(timerfd_settime(timerFd_,0,&timer,nullptr)<0){
        // Logger::instance().error("heartbeat timerfd_settime failed");
        close(timerFd_);
        timerFd_=-1;
        return false;
    }
    // Logger::instance().info("heartbeat timer started, interval=5s");
    return true;
}

void Heartbeat::stop(){
    if(timerFd_>=0){
        close(timerFd_);
        timerFd_=-1;
        // Logger::instance().info("heartbeat timer stopped");
    }
}

int Heartbeat::getTimerFd(){
    return timerFd_;
}

void Heartbeat::check(int fd){
    if(timerFd_<0) return;
    uint64_t exp=0;
    ssize_t n=read(timerFd_,&exp,sizeof(exp));
    if(n!=sizeof(exp)){
        // Logger::instance().error("heartbeat timerfd read failed");
        return;
    }
    json js;
    js["msgid"]=HEARTBEAT_MSG;
    string sendData=MessageCodec::encode(js.dump());
    if(!SocketUtil::sendAll(fd,sendData)){
        // Logger::instance().error("[heartbeat] send failed");
        return;
    }
    // Logger::instance().info("[heartbeat] send");
}