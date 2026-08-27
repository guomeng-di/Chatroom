#include "GroupMenu.h"
#include "../../ChatController/ChatController.h"
#include "../../Heartbeat/Heartbeat.h"
#include "../../../protocol/MessageCodec/MessageCodec.h"
#include "../../../protocol/MsgId.h"
#include "../../../netlib/base/SocketUtil/SocketUtil.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <limits>
#include <cerrno>
#include "../Color.h"

using namespace std;
using json=nlohmann::json;

void GroupMenu::run(int fd,const string& username){
    while(true){
        cout<<COLOR_BLUE;
        cout<<R"(
+--------------------------------+
|          群聊管理              |
+--------------------------------+
| 1.  创建群                     |
| 2.  加入群                     |
| 3.  聊天                       |
| 4.  查看群成员                 |
| 5.  退出群                     |
| 6.  踢出成员                   |
| 7.  解散群                     |
| 8.  添加管理员                 |
| 9.  删除管理员                 |
| 10. 查看群申请                 |
| 11. 处理群申请                 |
| 12. 查看我的群                 |
| 13. 群聊记录                   |
| 14. 邀请进群                   |
| 0.  返回                       |
+--------------------------------+
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
        if(!(cin>>cmd)){
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(),'\n');
            cout<<COLOR_RED;
            cout<<endl<<"输入错误，请输入数字"<<endl;
            cout<<COLOR_RESET;
            continue;
        }

        if(cmd==0) break;

        if(cmd==1){
            string groupName;
            int inviteCount;
            cout<<"群名称:";
            cin>>groupName;
            cout<<"邀请人数(至少1人):";

            cin>>inviteCount;
            if(cin.fail()||inviteCount<1){
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(),'\n');
                cout<<COLOR_RED<<"创建群至少需要邀请1人"<<COLOR_RESET<<endl;
                continue;
            }
            json js;
            js["msgid"]=CREATE_GROUP_MSG;
            js["username"]=username;
            js["groupname"]=groupName;
            js["members"]=json::array();
            for(int i=0;i<inviteCount;i++){
                string member;
                cout<<"第"<<i+1<<"位邀请用户:";
                cin>>member;
                js["members"].push_back(member);
            }
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==2){
            string groupName;
            cout<<"群名称:";
            cin>>groupName;
            json js;
            js["msgid"]=JOIN_GROUP_MSG;
            js["username"]=username;
            js["groupname"]=groupName;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==3){
            ChatController::groupChat(fd,username);
        }

        else if(cmd==4){
            string groupName;
            cout<<"群名称:";
            cin>>groupName;
            json js;
            js["msgid"]=GROUP_MEMBER_MSG;
            js["groupname"]=groupName;
            js["username"]=username;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==5){
            string groupName;
            cout<<"群名称:";
            cin>>groupName;
            json js;
            js["msgid"]=LEAVE_GROUP_MSG;
            js["username"]=username;
            js["groupname"]=groupName;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==6){
            string groupName;
            string member;
            cout<<"群名称:";
            cin>>groupName;
            cout<<"成员:";
            cin>>member;
            json js;
            js["msgid"]=KICK_MEMBER_MSG;
            js["groupname"]=groupName;
            js["operator"]=username;
            js["username"]=member;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==7){
            string groupName;
            cout<<"群名称:";
            cin>>groupName;
            json js;
            js["msgid"]=DELETE_GROUP_MSG;
            js["groupname"]=groupName;
            js["operator"]=username;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==8){
            string groupName;
            string admin;
            cout<<"群名称:";
            cin>>groupName;
            cout<<"管理员:";
            cin>>admin;
            json js;
            js["msgid"]=ADD_GROUP_ADMIN_MSG;
            js["groupname"]=groupName;
            js["operator"]=username;
            js["username"]=admin;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==9){
            string groupName;
            string admin;
            cout<<"群名称:";
            cin>>groupName;
            cout<<"管理员:";
            cin>>admin;
            json js;
            js["msgid"]=REMOVE_GROUP_ADMIN_MSG;
            js["groupname"]=groupName;
            js["operator"]=username;
            js["username"]=admin;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==10){
            string groupName;
            cout<<"群名称:";
            cin>>groupName;
            json js;
            js["msgid"]=GET_GROUP_REQUEST_MSG;
            js["groupname"]=groupName;
            js["operator"]=username;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==11){
            string groupName;
            string user;
            int accept;
            cout<<"群名称:";
            cin>>groupName;
            cout<<"申请人:";
            cin>>user;
            cout<<"1同意 0拒绝:";
            cin>>accept;
            json js;
            js["msgid"]=HANDLE_GROUP_REQUEST_MSG;
            js["groupname"]=groupName;
            js["operator"]=username;
            js["username"]=user;
            js["accept"]=accept;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==12){
            json js;
            js["msgid"]=GROUP_LIST_MSG;
            js["username"]=username;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==13){
            string groupName;
            cout<<"群名称:";
            cin>>groupName;
            json js;
            js["msgid"]=GET_GROUP_HISTORY;
            js["groupname"]=groupName;
            js["beforeId"]=0;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }

        else if(cmd==14){
            string groupName;
            string member;
            cout<<"群名称:";
            cin>>groupName;
            cout<<"邀请用户:";
            cin>>member;
            json js;
            js["msgid"]=INVITE_GROUP_MSG;
            js["groupname"]=groupName;
            js["operator"]=username;
            js["username"]=member;
            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);
        }
    }
}