#include "ChatController.h"
#include "../../protocol/MsgId.h"
#include "../../protocol/MessageCodec/MessageCodec.h"
#include <iostream>
#include "../menu/Color.h"
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <limits>
#include <sys/socket.h>
using namespace std;
using json=nlohmann::json;

void ChatController::privateChat(int fd,const string& username){
    string friendName;
    cout<<"好友账号:";
    cin>>friendName;
    // 清除换行
    cin.ignore(numeric_limits<streamsize>::max(),'\n');

cout<<COLOR_GREEN;
cout<<R"(

+--------------------------------+
|                                |
|            私聊模式             |
|                                |
+--------------------------------+

)";

cout<<"当前好友:"<<friendName<<endl;
cout<<"输入 quit 返回"<<endl;
cout<<COLOR_RESET;


        cout<<COLOR_GREEN;
        cout<<"我: ";
        cout<<COLOR_RESET;
    while(true){
        string message;
        getline(cin,message);


        if(message=="quit"){
            cout<<"退出聊天"<<endl;
            break;
        }
        if(message.empty()) continue;

        json js;
        js["msgid"]=CHAT_MSG;
        js["from"]=username;
        js["to"]=friendName;
        js["message"]=message;

        string data=MessageCodec::encode(js.dump());
        int ret=send(fd,data.data(),data.size(),0);
        if(ret<=0){
            cout<<"发送失败"<<endl;
            break;
        }
    }
    cout<<COLOR_GREEN;
    cout<<"已退出与 "<<friendName<<" 的聊天"<<endl;
    cout<<COLOR_RESET;
}

void ChatController::groupChat(int fd,const string& username){
    string groupName;
    cout<<"群名称:";
    cin>>groupName;

    cin.ignore(numeric_limits<streamsize>::max(),'\n');
     cout<<COLOR_BLUE;


    cout<<R"(

+--------------------------------+
|                                |
|             群聊模式            |
|                                |
+--------------------------------+

)";
    cout<<"当前群: "<<groupName<<endl;
    cout<<"输入 quit 返回"<<endl;
    cout<<COLOR_RESET;
 

        cout<<COLOR_GREEN;
        cout<<"我: ";
        cout<<COLOR_RESET;
    while(true){
        string message;
        getline(cin,message);

        if(message=="quit"){
            cout<<"退出聊天"<<endl;
            break;
        }
        if(message.empty())continue;
        json js;
        js["msgid"]=GROUP_CHAT_MSG;
        js["groupname"]=groupName;
        js["from"]=username;
        js["message"]=message;

        string data=MessageCodec::encode(js.dump());
        int ret=send(fd,data.data(),data.size(),0);
        if(ret<=0){
            cout<<"发送失败"<<endl;
            break;
        }

    }
    cout<<COLOR_BLUE;
    cout<<"已退出群聊聊天"<<groupName<<endl;
    cout<<COLOR_RESET;

}