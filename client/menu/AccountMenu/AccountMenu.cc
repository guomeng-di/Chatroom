#include "AccountMenu.h"
#include "../../../protocol/MessageCodec/MessageCodec.h"
#include "../../../protocol/MsgId.h"
#include "../Color.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <sys/socket.h>
using namespace std;
using json=nlohmann::json;
bool AccountMenu::sendVerifyCode(int fd,const string& email){
    json js;
    js["msgid"]=SEND_VERIFY_CODE_MSG;
    js["email"]=email;

    string data=MessageCodec::encode(js.dump());
    send(fd,data.data(),data.size(),0);
    return true;
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

int cmd;
cout<<"command:";
if(!(cin>>cmd)){
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(),'\n');
    cout<<COLOR_RED;
    cout<<endl<<"输入错误，请输入数字"<<endl;
    cout<<COLOR_RESET;
    break;
}

if(cmd==0)break;
//退出登录
else if(cmd==1){
    json js;
    js["msgid"]=LOGOUT_MSG;
    js["username"]=username;

    string data=MessageCodec::encode(js.dump());
    send(fd,data.data(),data.size(),0);
    cout<<"退出登录成功"<<endl;
    break;
}
//注销账号
else if(cmd==2){
    string password;
    cout<<"确认密码:";
    cin>>password;

    json js;
    js["msgid"]=DELETE_ACCOUNT_MSG;
    js["username"]=username;
    js["password"]=password;

    string data=MessageCodec::encode(js.dump());
    send(fd,data.data(),data.size(),0);
    cout<<"注销请求已发送"<<endl;
    break;
}
//修改密码
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
    send(fd,data.data(),data.size(),0);
    cout<<"修改密码请求已发送"<<endl;
}
}
}