#include "MessageCodec.h"
#include <string.h>
#include <netinet/in.h>
using namespace std;

MessageCodec::MessageCodec(){

}
MessageCodec::~MessageCodec(){

}
string MessageCodec::encode(const string& msg){
    uint32_t len=msg.size();
    u_int32_t netLen=htonl(len);
    string result;
    result.append((char*)&netLen,4);
    result+=msg;
    return result;
}
string MessageCodec::decode(const string& data){
    if(data.size()<4)return "";
    return string(data.data()+4,data.size()-4);
}