#include "FriendMenu.h"
#include <iostream>
#include <string>
#include <limits>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <cerrno>
#include "../../../protocol/MsgId.h"
#include "../../../protocol/MessageCodec/MessageCodec.h"
#include "../../../netlib/base/SocketUtil/SocketUtil.h"
#include "../../ChatController/ChatController.h"
#include "../../Heartbeat/Heartbeat.h"
#include "../Color.h"
#include <nlohmann/json.hpp>

using namespace std;
using json=nlohmann::json;

void FriendMenu::run(int fd,const string& username){
    while(true){
        cout<<COLOR_BLUE;
        cout<<R"(
+----------------+
|    好友管理     |
+----------------+
|1. 私聊         |
|2. 好友列表      |
|3. 删除好友      |
|4. 添加好友      |
|5. 好友申请      |
|6. 处理好友申请   |
|7. 屏蔽好友      |
|8. 取消屏蔽      |
|9. 私聊记录      |
|0. 返回         |
+----------------+
)";
        cout<<COLOR_RESET;

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO,&readfds);

        int selectRet=select(STDIN_FILENO+1,&readfds,nullptr,nullptr,nullptr);

        if(selectRet<0){
            if(errno==EINTR) continue;
            cerr<<"select failed"<<endl;
            break;
        }

        if(!FD_ISSET(STDIN_FILENO,&readfds)) continue;

        int cmd;
        cout<<"command:";
        if(!(cin>>cmd)){
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(),'\n');
            cout<<COLOR_RED;
            cout<<endl<<"输入错误，请输入数字"<<endl;
            cout<<COLOR_RESET;
            continue;
        }

        if(cmd==0) break;

        else if(cmd==1){
            ChatController::privateChat(fd,username);
        }

        else if(cmd==2){
            json js;
            js["msgid"]=FRIEND_LIST_MSG;
            js["username"]=username;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==3){
            string name;
            cout<<"删除好友:";
            cin>>name;
            json js;
            js["msgid"]=DELETE_FRIEND_MSG;
            js["username"]=username;
            js["friendname"]=name;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==4){
            string name;
            cout<<"好友名字:";
            cin>>name;
            json js;
            js["msgid"]=SEND_FRIEND_REQUEST_MSG;
            js["fromname"]=username;
            js["toname"]=name;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==5){
            json js;
            js["msgid"]=GET_FRIEND_REQUEST_MSG;
            js["username"]=username;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==6){
            string fromname;
            int action;
            cout<<"申请人:";
            cin>>fromname;
            cout<<"1.同意 0.拒绝:";
            cin>>action;
            json js;
            js["msgid"]=HANDLE_FRIEND_REQUEST_MSG;
            js["fromname"]=fromname;
            js["toname"]=username;
            js["action"]=action;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==7){
            string name;
            cout<<"屏蔽好友:";
            cin>>name;
            json js;
            js["msgid"]=ADD_BLOCK_MSG;
            js["username"]=username;
            js["blockname"]=name;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==8){
            string name;
            cout<<"解除屏蔽:";
            cin>>name;
            json js;
            js["msgid"]=REMOVE_BLOCK_MSG;
            js["username"]=username;
            js["blockname"]=name;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==9){
            string friendName;
            cout<<"好友:";
            cin>>friendName;
            json js;
            js["msgid"]=GET_PRIVATE_HISTORY;
            js["user1"]=username;
            js["user2"]=friendName;
            js["beforeId"]=0;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }
    }
}