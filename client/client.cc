#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <atomic>
#include <string>
#include <limits>
#include "ClientMessageHandler/ClientMessageHandler.h"
#include "../protocol/MessageCodec/MessageCodec.h"
#include "../protocol/MsgId.h"
#include "../manager/RedisManager/RedisManager.h"
#include <nlohmann/json.hpp>
#include "../service/ResetPasswordService/ResetPasswordService.h"
#include "../service/VerifyCodeService/VerifyCodeService.h"
using namespace std;
using json=nlohmann::json;
string username;
void recvMessage(int fd){
    char buf[1024];
    while(1){
        int len=recv(fd,buf,sizeof(buf),0);
        if(len<=0){
            cout<<"server close"<<endl;
            break;
        }
        string response(buf,len);

        string jsonStr=MessageCodec::decode(response);//提取出纯净的JSON字符串
        if(jsonStr.empty()){
          cout<<"empty response"<<endl;
          continue;
}
        json js=json::parse(jsonStr);//把文本字符串{"msgid":2,"username":"tom"}转换成程序可以操作的json对象js
        ClientMessageHandler::handle(js);
    }
}
bool login(int fd){
    //bool loginSuccess=false;
    //cin.clear();
    //cin.ignore(numeric_limits<streamsize>::max(), '\n');
//1输入
    cout<<"username:"; cin>>username;
    string password;
    cout<<"password:";cin>>password;
//2构造登录json语句,发送
    string loginJson="{"
    "\"msgid\":"+to_string(LOGIN_MSG)+","
    "\"username\":\""+username+"\","
    "\"password\":\""+password+"\""
    "}";
    string data=MessageCodec::encode(loginJson);
    send(fd,data.data(),data.size(),0);
//3接收
while(1){
    char buf[1024];
    int len=recv(fd,buf,sizeof(buf),0);
    if(len<=0){
        cout<<"server close"<<endl;
        close(fd);
        return 0;
    }
    string response(buf,len);
    string result=MessageCodec::decode(response);
    if(result.empty()){
       cout<<"empty response"<<endl;
       continue;
}
    json js=json::parse(result);
    int msgid=js["msgid"];
    if(msgid==LOGIN_ACK){
        if(js["errno"]==0){
            cout<<"login success"<<endl;
            return 1;
             //loginSuccess=true;
            
        }else{
            cout<<"login fail"<<endl;
            //close(fd);
            return 0;
        }
    }else if(msgid==FRIEND_REQUEST_NOTIFY){
    cout<<"\n==========好友申请=========="<<endl;
    cout<<js["message"]<<endl;
    cout<<"============================"<<endl;
}else if(msgid==OFFLINE_MSG){
    cout<<"\n\n==========离线消息=========="<<endl;
    cout<<js["message"]<<endl;
    cout<<"============================"<<endl;
    } else if(msgid==GROUP_OFFLINE_NOTIFY){
    cout<<"\n\n==========群离线消息=========="<<endl;
    cout<<js["message"]<<endl;
    cout<<"============================"<<endl;
    } else if(msgid == GROUP_OFFLINE_NOTIFY){
    cout<<"\n\n==========群离线消息=========="<<endl;
    string msg=js["message"];
    cout<<msg<<endl;
    cout<<"============================"<<endl;
}else{
    cout<<"other message:"
        <<result
        <<endl;
 }}
    return 1;
}
bool sendVerifyCode(int fd,const string& email){
    json js;
    js["msgid"]=SEND_VERIFY_CODE_MSG;
    js["email"]=email;
    string data=MessageCodec::encode(js.dump());
    send(fd,data.data(),data.size(),0);
    char buf[1024];
    int len=recv(fd,buf,sizeof(buf),0);
    if(len<=0){
        cout<<"server close"<<endl;
        return 0;
    }
    string res(buf,len);
    string result=MessageCodec::decode(res);
    json js_=json::parse(result);
    if(js_["errno"]==0){
        cout<<"send verify code success"<<endl;
        return 1;
    }else{
        cout<<"send verify code failed:"<<js_["message"]<<endl;
        return 0;
    }
}
bool registerUser(int fd){
    //1输入
    cout<<"username:"; cin>>username;
    string email;
    cout<<"your email:";cin>>email;

    //请求验证码
    sendVerifyCode(fd,email);
    //2 输入验证码
    string code;
    cout<<"verify code:";cin>>code;
    //3 输入密码
    string password;
    cout<<"password:";
    cin>>password;

    //2构造注册json语句,发送
    string loginJson="{"
    "\"msgid\":"+to_string(REGISTER_MSG)+","
    "\"username\":\""+username+"\","
    "\"email\":\""+email+"\","
    "\"code\":\""+code+"\","
    "\"password\":\""+password+"\""
    "}";
    string data=MessageCodec::encode(loginJson);
    send(fd,data.data(),data.size(),0);
    //3接收
    char buf[1024];
    int len=recv(fd,buf,sizeof(buf),0);
    if(len<=0){
        cout<<"server close"<<endl;
        close(fd);
        return 0;
    }
    string response(buf,len);
    string result=MessageCodec::decode(response);
    if(result.empty()){
        cout<<"empty response"<<endl;
}
    json js=json::parse(result);
    int msgid=js["msgid"];
    if(msgid==REGISTER_ACK){
        if(js["errno"]==0){
            cout<<"register success"<<endl;
            return 1;
        }else{
            cout<<"register fail"<<endl;
            //close(fd);
            return 0;
        }
    }
    return 1;
}
bool ResetPassword(int fd){
    json js;
    js["msgid"]=RESET_PASSWORD_MSG;
    string email,code,password;
    cout<<"your email:";cin>>email;
    //请求验证码
    sendVerifyCode(fd,email);
    cout<<"your verify code:";cin>>code;
    
    cout<<"your new password:";cin>>password;
    js["email"]=email;
    js["code"]=code;
    js["password"]=password;
    string data=MessageCodec::encode(js.dump());
    send(fd,data.data(),data.size(),0);
    return 1;
}
int main(){
    //1socket
    int fd=socket(AF_INET,SOCK_STREAM,0);
    //2bind
    sockaddr_in server{};
    server.sin_family=AF_INET;
    server.sin_port=htons(8888);
    inet_pton(AF_INET,"127.0.0.1",&server.sin_addr);
    if(connect(fd,(sockaddr*)&server,sizeof(server))<0){
        perror("connect");
        return -1;
    }
    cout<<"connect success"<<endl;

    

//2比对
while(true){

    //用户选择登录/注册->错了一直循环
    cout<<"=================="<<endl;
    cout<<"1 login"<<endl;
    cout<<"2 register"<<endl;
    cout<<"30 reset password"<<endl;
    cout<<"=================="<<endl;
    int choice;cin>>choice;
    if(choice==1){
        if(login(fd)) break;
    }
    else if(choice==2){
        if(registerUser(fd)&&login(fd)) break;
    }
    else if(choice==3){
        if(ResetPassword(fd)) break;
    }
    

}


    //开线程
    thread t(recvMessage,fd);
    t.detach();

    //循环显示目录
    while(true) {
        cout<<"----------------------------------"<<endl;
        cout<<"\n1 chat"<<endl;
        //cout<<"2 add friend"<<endl;
        cout<<"3 friend list"<<endl;
        cout<<"4 delete friend"<<endl;
        cout<<"5 group list"<<endl;
        cout<<"6 send friend request"<<endl;
        cout<<"7 view request"<<endl;
        cout<<"8 accept/reject"<<endl;
        cout<<"9 create group"<<endl;
        cout<<"10 apply join group"<<endl;
        cout<<"11 group chat"<<endl;
        cout<<"12 group member"<<endl;
        cout<<"13 leave group"<<endl;
        //cout<<"14 login"<<endl;
        //cout<<"15 register"<<endl;
        cout<<"16 logout"<<endl;
        cout<<"19 delete account"<<endl;
        cout<<"20 get private history"<<endl;
        cout<<"21 get group history"<<endl;
        cout<<"22 kick member"<<endl;
        cout<<"23 delete group"<<endl;
        cout<<"24 add group admin"<<endl;
        cout<<"25 delete group admin"<<endl;
        cout<<"26 view group request"<<endl;
        cout<<"27 handle group request"<<endl;
        cout<<"28 send verify code"<<endl;
        cout<<"29 heartbeat"<<endl;
        cout<<"30 reset password"<<endl;
        cout<<"31 block friend"<<endl;
        cout<<"32 unblock friend"<<endl;
        cout<<"----------------------------------"<<endl;
        

        cout<<"command:";
        
        int cmd; cin>>cmd;
        if(cin.fail()){
           cout<<"input error"<<endl;
           cin.clear();
           cin.ignore(1024,'\n');
           continue;
        }
        
        //私聊
        if(cmd==CHAT_MSG){
            string to,msg;
            cout<<"to:";cin>>to;
            cout<<"message:";
            //cin.ignore();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            getline(cin,msg);

            string json="{"
            "\"msgid\":"+to_string(CHAT_MSG)+","
            "\"from\":\""+username+"\","
            "\"to\":\""+to+"\","
            "\"message\":\""+msg+"\""
            "}";

            cout<<"send json:"<<json<<endl;

            string sendData=MessageCodec::encode(json);
            int n=send(fd,sendData.data(),sendData.size(),0);
            cout<<"send bytes="<<n<<endl;
        }
        
        // //添加好友
        // else if(cmd==ADD_FRIEND_MSG){
        //     string friendName;
        //     cout<<"friend:"; cin>>friendName;
        //     cout<<"friendName=["<<friendName<<"]"<<endl;

        //     string json="{"
        //     "\"msgid\":"+to_string(ADD_FRIEND_MSG)+","
        //     "\"username\":\""+username+"\","
        //     "\"friendname\":\""+friendName+"\""
        //     "}";

        //     string sendData=MessageCodec::encode(json);
        //     send(fd,sendData.data(),sendData.size(),0);
        // }

        //查询好友列表
        else if(cmd==FRIEND_LIST_MSG){
            string json="{"
            "\"msgid\":"+to_string(FRIEND_LIST_MSG)+","
            "\"username\":\""+username+"\""
            "}";

            string sendData= MessageCodec::encode(json);

            send(fd,sendData.data(),sendData.size(),0);
        }
        
        //删除好友
        else if(cmd==DELETE_FRIEND_MSG){
            string friendName;
            cout<<"delete friend:"; cin>>friendName;

            string json="{"
            "\"msgid\":"+to_string(DELETE_FRIEND_MSG)+","
            "\"username\":\""+username+"\","
            "\"friendname\":\""+friendName+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
        }

        //查看加了哪些群
        else if(cmd==GROUP_LIST_MSG){
            string json="{"
            "\"msgid\":"+to_string(GROUP_LIST_MSG)+","
            "\"username\":\""+username+"\""
            "}";
            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
        }

        //发送好友申请
        else if(cmd==SEND_FRIEND_REQUEST_MSG){
            string friendName;
            cout<<"apply friend: "; cin>>friendName;

            string json="{"
            "\"msgid\":"+to_string(SEND_FRIEND_REQUEST_MSG)+","
            "\"fromname\":\""+username+"\","
            "\"toname\":\""+friendName+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
        }

        //查看好友申请
        else if(cmd==GET_FRIEND_REQUEST_MSG){
            string json="{"
            "\"msgid\":"+to_string(GET_FRIEND_REQUEST_MSG)+","
            "\"username\":\""+username+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
        }

        //处理好友申请(同意/拒绝)
        else if(cmd==HANDLE_FRIEND_REQUEST_MSG){
            string from;
            int action;
            cout<<"申请人:";cin>>from;
            cout<<"1同意 0拒绝:";cin>>action;

            string json="{"
            "\"msgid\":"+to_string(HANDLE_FRIEND_REQUEST_MSG)+","
            "\"fromname\":\""+from+"\","
            "\"toname\":\""+username+"\","
            "\"action\":"+to_string(action)+
            "}";
            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
        }

        //创建群
        else if(cmd==CREATE_GROUP_MSG){
            string groupName;
            cout<<"group name:";cin>>groupName;

            string json="{"
            "\"msgid\":"+to_string(CREATE_GROUP_MSG)+","
            "\"username\":\""+username+"\","
            "\"groupname\":\""+groupName+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
        }

        //加入群
        else if(cmd==JOIN_GROUP_MSG){
            string groupName;
            cout<<"group name:";cin>>groupName;

            string json="{"
            "\"msgid\":"+to_string(JOIN_GROUP_MSG)+","
            "\"username\":\""+username+"\","
            "\"groupname\":\""+groupName+"\""
            "}";
        
            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
        }

        //群聊发送消息
        else if(cmd==GROUP_CHAT_MSG){
            string groupName,message;
            cout<<"group:";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            getline(cin,groupName);
            cout<<"message:";
            //cin.ignore(numeric_limits<streamsize>::max(), '\n');
            getline(cin,message);

            string json="{"
            "\"msgid\":"+to_string(GROUP_CHAT_MSG)+","
            "\"groupname\":\""+groupName+"\","
            "\"from\":\""+username+"\","
            "\"message\":\""+message+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
        }

        //查看群成员
        else if(cmd==GROUP_MEMBER_MSG){
            string groupName;
            cout<<"group:";cin>>groupName;

            string json="{"
            "\"msgid\":"+to_string(GROUP_MEMBER_MSG)+","
            "\"groupname\":\""+groupName+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);

        }

        //退出群
        else if(cmd==LEAVE_GROUP_MSG){
            string groupName;
            cout<<"group:";cin>>groupName;

            string json="{"
            "\"msgid\":"+to_string(LEAVE_GROUP_MSG)+","
            "\"username\":\""+username+"\","
            "\"groupname\":\""+groupName+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
        }


        //查看某人参加的群
        else if(cmd==GROUP_LIST_MSG){
            string json="{"
            "\"msgid\":"+to_string(GROUP_LIST_MSG)+","
            "\"username\":\""+username+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
        }

        
        
        //主动退出登录
        else if(cmd==LOGOUT_MSG){
            string json="{"
            "\"msgid\":"+to_string(LOGOUT_MSG)+","
            "\"username\":\""+username+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
        }
        

        //账号注销
        else if(cmd==DELETE_ACCOUNT_MSG){
            string password;
            cout<<"confirm password:"; cin>>password;
            string json="{"
            "\"msgid\":"+to_string(DELETE_ACCOUNT_MSG)+","
            "\"username\":\""+username+"\","
            "\"password\":\""+password+"\""
            "}";
            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);

        }

        //查看私聊聊天记录
        else if(cmd==GET_PRIVATE_HISTORY){
            string friendName;
            cout<<"friend username:";cin>>friendName;

            string json="{"
            "\"msgid\":"+to_string(GET_PRIVATE_HISTORY)+","
            "\"user1\":\""+username+"\","
            "\"user2\":\""+friendName+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
}

        //查看群聊聊天记录
        else if(cmd==GET_GROUP_HISTORY){
            string groupName;
            cout<<"group name:";cin>>groupName;

            string json="{"
            "\"msgid\":"+to_string(GET_GROUP_HISTORY)+","
            "\"groupname\":\""+groupName+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
}

        //踢人
        else if(cmd==KICK_MEMBER_MSG){
            string groupName,kick_member;
            cout<<"group name:";
            cin>>groupName;
            cout<<"kick member:";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            getline(cin,kick_member);

            string json="{"
            "\"msgid\":"+to_string(KICK_MEMBER_MSG)+","
            "\"groupname\":\""+groupName+"\","
            "\"operator\":\""+username+"\","
            "\"username\":\""+kick_member+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
}

        //添加群管理员
        else if(cmd==ADD_GROUP_ADMIN_MSG){
            string groupName,admin;
            cout<<"group name:";
            cin>>groupName;
            cout<<"set admin:";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            getline(cin,admin);

            string json="{"
            "\"msgid\":"+to_string(ADD_GROUP_ADMIN_MSG)+","
            "\"groupname\":\""+groupName+"\","
            "\"operator\":\""+username+"\","
            "\"username\":\""+admin+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
}

        //删除群管理员
        else if(cmd==REMOVE_GROUP_ADMIN_MSG){
            string groupName,admin;
            cout<<"group name:";
            cin>>groupName;
            cout<<"delete admin:";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            getline(cin,admin);

            string json="{"
            "\"msgid\":"+to_string(REMOVE_GROUP_ADMIN_MSG)+","
            "\"groupname\":\""+groupName+"\","
            "\"operator\":\""+username+"\","
            "\"username\":\""+admin+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
}

        //解散群
        else if(cmd==DELETE_GROUP_MSG){
            string groupName;
            cout<<"group name:";
            cin>>groupName;

            string json="{"
            "\"msgid\":"+to_string(DELETE_GROUP_MSG)+","
            "\"operator\":\""+username+"\","
            "\"groupname\":\""+groupName+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
}

        //查看群聊申请列表
        else if(cmd==GET_GROUP_REQUEST_MSG){
            string groupName;
            cout<<"group name:";
            cin>>groupName;

            string json="{"
            "\"msgid\":"+to_string(GET_GROUP_REQUEST_MSG)+","
            "\"operator\":\""+username+"\","
            "\"groupname\":\""+groupName+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
}

        //处理群聊申请列表
        else if(cmd==HANDLE_GROUP_REQUEST_MSG){
            string groupName,handle_user;
            bool accept;
            cout<<"group name:";cin>>groupName;
            cout<<"username:";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin>>handle_user;
            cout<<"1-accept  0-reject:";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin>>accept;
            string json="{"
            "\"msgid\":"+to_string(HANDLE_GROUP_REQUEST_MSG)+","
            "\"groupname\":\""+groupName+"\","
            "\"operator\":\""+username+"\","
            "\"username\":\""+handle_user+"\","
            "\"accept\":"+to_string(accept)+
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
}

        //请求发送验证码
        else if(cmd==SEND_VERIFY_CODE_MSG){
            string email;
            cout<<"email:";
            cin>>email;

            string json="{"
            "\"msgid\":"+to_string(SEND_VERIFY_CODE_MSG)+","
            "\"email\":\""+email+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
}

        //心跳检测
        else if(cmd==HEARTBEAT_MSG){
            string json="{"
            "\"msgid\":"+to_string(HEARTBEAT_MSG)+"}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
}

        //重置密码
        else if(cmd==RESET_PASSWORD_MSG){
            string email,code,password;
            cout<<"your email:";cin>>email;
            cout<<"your verify code:";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin>>code;
            cout<<"your new password:";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin>>password;
            string json=
            "{"
            "\"msgid\":"+to_string(RESET_PASSWORD_MSG)+","
            "\"email\":\""+email+"\","
            "\"code\":\""+code+"\","
            "\"password\":\""+password+"\""
            "}";


            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
}

        //屏蔽好友
        else if(cmd==ADD_BLOCK_MSG){
            string blockname;
            cout<<"block name:";
            cin>>blockname;

            string json="{"
            "\"msgid\":"+to_string(ADD_BLOCK_MSG)+","
            "\"username\":\""+username+"\","
            "\"blockname\":\""+blockname+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
}

        //取消屏蔽好友
        else if(cmd==REMOVE_BLOCK_MSG){
            string unblockname;
            cout<<"remove block name:";
            cin>>unblockname;

            string json="{"
            "\"msgid\":"+to_string(REMOVE_BLOCK_MSG)+","
            "\"username\":\""+username+"\","
            "\"blockname\":\""+unblockname+"\""
            "}";

            string sendData=MessageCodec::encode(json);
            send(fd,sendData.data(),sendData.size(),0);
}

   }  
    close(fd);
    return 0;
}