#include "MessageCodec.h"
#include <string.h>
#include <netinet/in.h>
#include <iostream>
#include <arpa/inet.h>
using namespace std;

MessageCodec::MessageCodec(){}
MessageCodec::~MessageCodec(){}
// |totalLen(4)|msgid(4)|json|
string MessageCodec::encode(const string& msg){
    json js=json::parse(msg);//msg是序列化的,js是反序列化的
    int msgid=(int)js["msgid"];
    uint32_t totalLen=4+msg.size();//msgid+json
    uint32_t totalLen_=htonl(totalLen),netMsgid=htonl(msgid);//多字节整数在网络上传输,必须转大端
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

// | totalLen(4) | msgid(4) | jsonLen(4) | json | binary |
string MessageCodec::encodeBinary(int msgid,const json& js,const string& data){
    string jsonStr=js.dump();
    size_t jsonLen=jsonStr.size();
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
// bool MessageCodec::decodeHeader(const string& data,int& msgid,int& bodyLen){
//     if(data.size()<8) return 0;
//     int totalLen;
//     memcpy(&totalLen,data.data(),4);
//     memcpy(&msgid,data.data()+4,4);
//     bodyLen=totalLen-4;
//     return 1;
// }
int MessageCodec::getMsgId(const string& data){
    int msgid;
    memcpy(&msgid,data.data(),4);
    return ntohl(msgid);
}
FilePacket MessageCodec::decodeBinary(const string& msg){
    FilePacket packet;
    cout<<"decode binary size="<<msg.size()<<endl;
    if(msg.size() < 8){
        throw runtime_error("binary packet too small");
    }
    int msgid;
    memcpy(&msgid,msg.data(),4);//跳过长度4字节
    //msgid:
    packet.msgid=ntohl(msgid);

    size_t jsonLen;
    memcpy(&jsonLen,msg.data()+4,4);//获取json长度
    jsonLen=ntohl(jsonLen);

    cout << "binary msgid="<< packet.msgid<< endl;
    cout << "jsonLen="<< jsonLen<< endl;
    // 3. 检查 jsonLen
    if(jsonLen > msg.size()- 8)
    {
        throw runtime_error("invalid binary json length");
    }
    // 4. 解析 JSON
    string jsonStr( msg.data() + 8, jsonLen);
    try{
        packet.info = json::parse(jsonStr);
    }
    catch(const exception& e){
        cerr << "binary json parse failed:"<< e.what()<< endl;
        cerr << "jsonLen="<< jsonLen<< endl;
        cerr << "json="<< jsonStr<< endl;
        throw;
    }
    // 5. 取文件数据

    size_t dataOffset = 8 + jsonLen;
    size_t dataSize =msg.size() - dataOffset;
    packet.data.assign(msg.data() + dataOffset,dataSize);
    cout << "file data size="<< packet.data.size()<< endl;
    return packet;

}
// 在网络上传输时，只有多字节整数需要转大端(htonl)，字符串不用
// 为什么字符串不用？
// std::string /char 数组本质就是一字节一字节的原始字节序列
// 比如 "hello" 就是依次：0x68 0x65 0x6c 0x6c 0x6f

// 为什么 uint32_t /int 这种多字节整数必须转？
// 比如 uint32_t num = 0x12345678
// 小端机器（x86、咱们电脑）内存存放顺序：78 56 34 12
// 网络约定统一用大端：12 34 56 78