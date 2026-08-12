#include "MessageCodec.h"
#include <string.h>
#include <netinet/in.h>
#include <iostream>
#include <arpa/inet.h>
using namespace std;

MessageCodec::MessageCodec(){

}
MessageCodec::~MessageCodec(){

}

// |totalLen|msgid|json|
//    4     |  4
string MessageCodec::encode(const string& msg){
    // uint32_t len=msg.size();
    // u_int32_t netLen=htonl(len);
    // string result;
    // result.append((char*)&netLen,4);
    // result+=msg;
    // return result;
    json js=json::parse(msg);
    int msgid=js["msgid"].get<int>();
    uint32_t totalLen=4+msg.size();
    uint32_t totalLen_=htonl(totalLen),netMsgid=htonl(msgid);
    string result;
    result.append((char*)&totalLen_,4);
    result.append((char*)&netMsgid, 4);
    result+=msg;
    return result;
}
string MessageCodec::decode(const string& data){
    if(data.size()<8)return "";
    return string(data.data()+8,data.size()-8);
}

// | totalLen | msgid | jsonLen | json | binary |
//    4         4        4
string MessageCodec::encodeBinary(int msgid,const json& js,const string& data){
    string jsonStr=js.dump();
    int jsonLen=jsonStr.size();
    int totalLen=4+4+jsonLen+data.size();//4:msgid   data.size():data
    uint32_t msgid_=htonl(msgid),totalLen_=htonl(totalLen),jsonLen_=htonl(jsonLen);
    string result;
    result.append((char*)&totalLen_,4);//长度
    result.append((char*)&msgid_,4);//msgid
    result.append((char*)&jsonLen_,4);//jsonLen
    result+=jsonStr;
    result+=data;
    return result;//json+二进制
}
bool MessageCodec::decodeHeader(const string& data,int& msgid,int& bodyLen){
    if(data.size()<8) return 0;
    int totalLen;
    memcpy(&totalLen,data.data(),4);
    memcpy(&msgid,data.data()+4,4);
    bodyLen=totalLen-4;
    return 1;
}
int MessageCodec::getMsgId(const string& data){
    int msgid;
    memcpy(&msgid,data.data(),4);
    return ntohl(msgid);
}
FilePacket MessageCodec::decodeBinary(const string& msg){
    FilePacket packet;

    cout<<"decode binary size="
        <<msg.size()
        <<endl;


    int msgid;
    memcpy(&msgid,msg.data(),4);//跳过长度4字节
    //msgid:
    packet.msgid=ntohl(msgid);

    int jsonLen;
    memcpy(&jsonLen,msg.data()+4,4);//获取json长度
    jsonLen=ntohl(jsonLen);

    cout<<"binary msgid="
        <<packet.msgid
        <<endl;


    cout<<"jsonLen="
        <<jsonLen
        <<endl;



    string jsonStr(msg.data()+8,jsonLen);

     cout<<"json="
        <<jsonStr
        <<endl;


    //info:
    packet.info=json::parse(jsonStr);

    //data:
    packet.data.assign(msg.data()+8+jsonLen,msg.size()-8-jsonLen);

    return packet;

}