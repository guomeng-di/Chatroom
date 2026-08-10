#include "Buffer.h"
#include <cstring>
#include <cstdint>
#include <arpa/inet.h>
using namespace std;

Buffer::Buffer(){
}
Buffer::~Buffer(){
}
void Buffer::append(const char* data,size_t len){
    buffer_.append(data,len);
}
string Buffer::retrieveAll(){
    string msg=buffer_;
    buffer_.clear();
    return msg;
}
size_t Buffer::size(){
    return buffer_.size();
}
const char* Buffer::peek(){
    return buffer_.data();
}
void Buffer::retrieve(size_t len){
    buffer_.erase(0,len);
}
bool Buffer::hasMessage(){
    if(size()<4){
        return 0;
    }
    size_t len=0;
    memcpy(&len,peek(),4);
    //memcpy(去哪里,从哪里复制,复制多少);这里将字符串的长度(前四个字节)复制给len
    //重点：网络序转本机字节序
    uint32_t body_len=ntohl(len);
    if(size()>=4+body_len) return 1;
    return 0;
}
string Buffer::retrieveMessage(){
    size_t len=0;
    memcpy(&len,peek(),4);
    uint32_t body_len=ntohl(len);
    string msg(buffer_.data()+4,body_len);//跳过4字节消息头
    buffer_.erase(0,body_len+4);
    return msg;
}