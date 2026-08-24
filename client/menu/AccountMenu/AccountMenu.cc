#include "AccountMenu.h"
#include "../../../protocol/MessageCodec/MessageCodec.h"
#include "../../../protocol/MsgId.h"
#include "../../../netlib/base/SocketUtil/SocketUtil.h"
#include "../../../netlib/base/Logger/Logger.h"
#include "../../Heartbeat/Heartbeat.h"
#include "../Color.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <limits>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <cerrno>

using namespace std;
using json=nlohmann::json;

bool AccountMenu::sendVerifyCode(int fd,const string& email){
    json js;
    js["msgid"]=SEND_VERIFY_CODE_MSG;
    js["email"]=email;
    string data=MessageCodec::encode(js.dump());
    return SocketUtil::sendAll(fd,data);
}

void AccountMenu::run(int fd,const string& username){
    while(true){
        cout<<COLOR_BLUE;
        cout<<R"(
+----------------------------+
|        账号管理            |
+----------------------------+
|1. 退出登录                 |
|2. 注销账号                 |
|3. 修改密码                 |
|0. 返回                     |
+----------------------------+
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
            json js;
            js["msgid"]=LOGOUT_MSG;
            js["username"]=username;

            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);

            cout<<"退出登录成功"<<endl;
            Heartbeat::stop();
            close(fd);
            exit(0);
        }

        else if(cmd==2){
            string password;
            cout<<"确认密码:";
            cin>>password;

            json js;
            js["msgid"]=DELETE_ACCOUNT_MSG;
            js["username"]=username;
            js["password"]=password;

            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);

            cout<<"注销请求已发送"<<endl;
            Heartbeat::stop();
            close(fd);
            exit(0);
        }

        else if(cmd==3){
            string email;
            string code;
            string password;

            cout<<"邮箱:";
            cin>>email;

            if(!sendVerifyCode(fd,email)){
                cout<<"验证码发送失败"<<endl;
                continue;
            }

            cout<<"验证码:";
            cin>>code;

            cout<<"新密码:";
            cin>>password;

            json js;
            js["msgid"]=RESET_PASSWORD_MSG;
            js["email"]=email;
            js["code"]=code;
            js["password"]=password;

            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);

            cout<<"修改密码请求已发送"<<endl;
        }
    }
}