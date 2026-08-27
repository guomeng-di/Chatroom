#include "ChatController.h"
#include "../../protocol/MsgId.h"
#include "../../protocol/MessageCodec/MessageCodec.h"
#include "../../netlib/base/SocketUtil/SocketUtil.h"
#include "../menu/Color.h"
#include "../../model/FriendModel/FriendModel.h"
#include "../../model/GroupModel/GroupModel.h"
#include "../Heartbeat/Heartbeat.h"
#include <iostream>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <limits>
#include <sys/socket.h>
#include <sys/select.h>
#include <termios.h>
#include <iconv.h>
#include <cerrno>
#include <deque>
#include <string>
#include <deque>
#include <mutex>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include "ChatInputReader/ChatInputReader.h"
#include "ChatInputMode/ChatInputMode.h"
using json=nlohmann::json;
using namespace std;
static bool isValidUTF8(const string& text){
    const unsigned char* data=(const unsigned char*)text.data();
    size_t len=text.size();
    size_t i=0;
    while(i<len){
        unsigned char c=data[i];
        if(c<=0x7F)
        {
            i++;
            continue;
        }
        int bytes=0;
        if((c&0xE0)==0xC0)
        {
            bytes=2;
        }
        else if((c&0xF0)==0xE0)
        {
            bytes=3;
        }
        else if((c&0xF8)==0xF0)
        {
            bytes=4;
        }
        else
        {
            return false;
        }
        if(i+bytes>len)
        {
            return false;
        }
        for(int j=1;j<bytes;j++)
        {
            if((data[i+j]&0xC0)!=0x80)
            {
                return false;
            }
        }
        i+=bytes;
    }
    return true;
}

string normalizeChatText(const string& text){
    if(text.empty())return text;
    
    string clean;
    clean.reserve(text.size());//预分配内存
    bool hasCR=false;
    for(char c:text){//遍历每一个字符
        if(c=='\r'){
            hasCR=true;
            continue;//windows为\r\n,linux为\n
        }
        clean.push_back(c);
    }if(clean.empty()){
        return clean;
    }if(!hasCR&&isValidUTF8(clean)){
        return clean;
    }if(isValidUTF8(clean)){
        return clean;
    }
    iconv_t converter=iconv_open("UTF-8","GB18030");
    if(converter==(iconv_t)-1){
        return clean;
    }
    size_t inputSize=clean.size();
    size_t outputSize=inputSize*4+4;
    string result(outputSize,'\0');
    char* inputBuffer=clean.data();
    char* outputBuffer=result.data();
    size_t remainOutput=outputSize;
    size_t ret=iconv(converter,&inputBuffer,&inputSize,&outputBuffer,&remainOutput);
    iconv_close(converter);
    if(ret==(size_t)-1){
        return clean;
    }
    result.resize(outputSize-remainOutput);
    return result;
}
//非阻塞读取标准输入

 void ChatController::privateChat(int fd,const string& username){
  string friendName;
  cout<<"好友账号:";
  cin>>friendName;
  cin.ignore(numeric_limits<streamsize>::max(),'\n');
  FriendModel model;
  if(!model.isFriend(username,friendName)){
   cout<<COLOR_RED<<"你们不是好友,不可发起私聊"<<COLOR_RESET<<endl;
   return;
  }
  cout<<COLOR_GREEN;
  cout<<R"(
 +--------------------------------+
 |                                |
 |            私聊模式             |
 |                                |
 +--------------------------------+
 )";
  cout<<"当前好友:"<<friendName<<endl;
  cout<<"发送方式:整体发送"<<endl;
  cout<<"输入 /mode 查看或切换发送方式"<<endl;
  cout<<"输入 quit 返回"<<endl;
  cout<<COLOR_RESET;
  ChatInputMode inputMode;
  if(!inputMode.active()){
   cout<<COLOR_RED<<"终端输入模式初始化失败"<<COLOR_RESET<<endl;
   return;
  }
  ChatInputReader inputReader(false);
  while(true){
   fd_set readfds;
   FD_ZERO(&readfds);
   FD_SET(STDIN_FILENO,&readfds);
   int ret=select(STDIN_FILENO+1,&readfds,nullptr,nullptr,nullptr);
   if(ret<0){
    if(errno==EINTR){
     continue;
    }
    cout<<endl<<COLOR_RED<<"select失败:"<<strerror(errno)<<COLOR_RESET<<endl;
    break;
   }
   if(!FD_ISSET(STDIN_FILENO,&readfds)){
    continue;
   }
   string message;
   while(inputReader.readMessage(message)){
    if(message.empty()){
     continue;
    }
    if(message=="quit"){
     cout<<endl<<"退出聊天"<<endl;
     goto EXIT_CHAT;
    }
    if(handleModeCommand(message,inputReader)){
     continue;
    }
    json js;
    js["msgid"]=CHAT_MSG;
    js["from"]=username;
    js["to"]=friendName;
    js["message"]=normalizeChatText(message);
    string data=MessageCodec::encode(js.dump(-1,' ',false,json::error_handler_t::replace));
    if(!SocketUtil::sendAll(fd,data)){
     cout<<endl<<COLOR_RED<<"发送失败"<<COLOR_RESET<<endl;
     goto EXIT_CHAT;
    }
   }
  }
 EXIT_CHAT:
  cout<<endl<<COLOR_GREEN<<"已退出与 "<<friendName<<" 的聊天"<<COLOR_RESET<<endl;
 }

void ChatController::groupChat(int fd,const string& username){
    string groupName;
    cout<<"群名称:";
    cin>>groupName;
    cin.ignore(numeric_limits<streamsize>::max(),'\n');
    GroupModel groupModel;
    if(!groupModel.groupExist(groupName)){
        cout<<COLOR_RED<<"发送群消息失败，群不存在"<<COLOR_RESET<<endl;
        return;
    }
    if(!groupModel.isMember(groupName,username)){
        cout<<COLOR_RED<<"不是群成员，无法发送群消息"<<COLOR_RESET<<endl;
        return;
    }
    cout<<COLOR_BLUE;
    cout<<R"(
+--------------------------------+
|                                |
|             群聊模式            |
|                                |
+--------------------------------+
    )";
    cout<<"当前群: "<<groupName<<endl;
    cout<<"发送方式:整体发送"<<endl;
    cout<<"输入 /mode 查看或切换发送方式"<<endl;
    cout<<"输入 quit 返回"<<endl;
    cout<<COLOR_RESET;
    ChatInputMode inputMode;
    ChatInputReader inputReader(false);
    while(true){
        fd_set readfds;
FD_ZERO(&readfds);
FD_SET(STDIN_FILENO,&readfds);
int selectRet=select(STDIN_FILENO+1,&readfds,nullptr,nullptr,nullptr);
        if(selectRet<0){
            if(errno==EINTR){
                continue;
            }
            cerr<<"select failed"<<endl;
            break;
        }
if(!FD_ISSET(STDIN_FILENO,&readfds)){
    continue;
}
string message;
        while(inputReader.readMessage(message)){
            if(message.empty()){
                continue;
            }
            if(message=="quit"){
                cout<<"退出聊天"<<endl;
                goto EXIT_GROUP;
            }
            if(handleModeCommand(message,inputReader)){
                continue;
            }
            json js;
            js["msgid"]=GROUP_CHAT_MSG;
            js["groupname"]=groupName;
            js["from"]=username;
            js["message"]=normalizeChatText(message);
            string data=MessageCodec::encode(js.dump(-1,' ',false,json::error_handler_t::replace));
            bool sendRet=SocketUtil::sendAll(fd,data);
            if(!sendRet){
                cout<<COLOR_RED<<"发送失败"<<COLOR_RESET<<endl;
                goto EXIT_GROUP;
            }
        }
    }
EXIT_GROUP:
    cout<<COLOR_BLUE;
    cout<<"已退出群聊 "<<groupName<<endl;
    cout<<COLOR_RESET;
}
