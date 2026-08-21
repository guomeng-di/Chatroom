// 选择私聊
//     |
// 输入好友
//     |
// 进入while
//     |
// 输入消息
//     |
// 判断quit
//     |
// 发送CHAT_MSG
//     |
// 继续等待
#pragma once
#include <string>
using namespace std;
class ChatController{
    public:
      // 私聊
      static void privateChat(int fd,const string& username);
      // 群聊
      static void groupChat(int fd,const string& username);
};