#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <sys/timerfd.h>
#include <sys/select.h>
#include <cstdint>
#include <algorithm>
#include <atomic>
#include <string>
#include "../netlib/base/Logger.h"
#include <limits>
#include "../src/config.h"
#include "ClientMessageHandler/ClientMessageHandler.h"
#include "../protocol/MessageCodec/MessageCodec.h"
#include "../protocol/MsgId.h"
#include "../manager/RedisManager/RedisManager.h"
#include "../manager/FileManager/FileManager.h"
#include <nlohmann/json.hpp>
#include "FileClient/FileClient.h"
#include "../netlib/net/Buffer/Buffer.h"
using namespace std;
using json=nlohmann::json;
string username;
Buffer clientBuffer;
string currentSendFile;
void sendHeartbeat(int fd){
    json js;
    js["msgid"]=HEARTBEAT_MSG;
    string sendData=MessageCodec::encode(js.dump());
    int n=send(fd,sendData.data(),sendData.size(),0);
    if(n<=0) cout << "[heartbeat] send failed" << endl;
    else cout << "\n[heartbeat] send" << endl;   
}
void recvMessage(int fd){
    char buf[1024];
    while(1){
        int len=recv(fd,buf,sizeof(buf),0);
        if(len<=0){
            cout<<"server close"<<endl;
            break;
        }
        clientBuffer.append(buf,len);
        while(clientBuffer.hasMessage()){
            string msg=clientBuffer.retrieveMessage();//返回msgid+json
            int msgid=MessageCodec::getMsgId(msg);
            if(msgid==FILE_DATA_MSG){
                FilePacket packet=MessageCodec::decodeBinary(msg);
                FileClient::instance().receiveFile(packet,fd);
            }else{
                string jsonStr(msg.data()+4,msg.size()-4);
                json js=json::parse(jsonStr);
                ClientMessageHandler::handle(js,fd);
            }
        }
    }
}
bool login(int fd){
    cout<<"登录:"<<endl;
//1输入
    cout<<"username:"; cin>>username;
    string password;
    cout<<"password:";
    //cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin>>password;
//2构造登录json语句,发送
    json loginMsg;
    loginMsg["msgid"]=LOGIN_MSG;
    loginMsg["username"]=username;
    loginMsg["password"]=password;
    string data=MessageCodec::encode(loginMsg.dump());
    int ret=send(fd,data.data(),data.size(),0);
    if(ret<=0){
        cout<<"send login message fail"<<endl;
        return 0;
    }

   while(1){
    char buf[1024];
    int len=recv(fd,buf,sizeof(buf),0);
    if(len<=0){
        cout<<"server close"<<endl;
        close(fd);
        return 0;
    }
    clientBuffer.append(buf,len);
    while(clientBuffer.hasMessage()){
        string msg=clientBuffer.retrieveMessage(); // 返回 msgid(4)+json
        int msgid=MessageCodec::getMsgId(msg);
        string jsonStr(msg.data()+4,msg.size()-4);
        json js=json::parse(jsonStr);

        if(msgid==LOGIN_ACK){
            if(js["errno"]==0){
                cout<<"login success"<<endl;
                FileClient::instance().setUsername(username);
                return 1; // buffer中剩余消息(含FILE_RESUME_NOTIFY)由recvMessage处理
            }else{
                cout<<"login fail"<<endl;
                return 0;
            }
        }
        // 登录阶段到达的其他通知，直接分发处理，避免丢失
        ClientMessageHandler::handle(js,fd);
    }
 
// //3接收
// bool loginSuccess=0;
// while(1){
//     char buf[1024];
//     int len=recv(fd,buf,sizeof(buf),0);
//     if(len<=0){
//         cout<<"server close"<<endl;
//         close(fd);
//         return 0;
//     }
//     string response(buf,len);
//     string result=MessageCodec::decode(response);
//     if(result.empty()){
//        cout<<"empty response"<<endl;
//        continue;
// }
//     json js=json::parse(result);
//     int msgid=js["msgid"];
//     if(msgid==LOGIN_ACK){
//         if(js["errno"]==0){
//             cout<<"login success"<<endl;
//             FileClient::instance().setUsername(username);
//             break;
//              loginSuccess=true;
            
//         }else{
//             cout<<"login fail"<<endl;
//             return 0;
//         }
// }else if(msgid==FRIEND_REQUEST_NOTIFY){
//     cout<<"\n==========好友申请=========="<<endl;
//     cout<<js["message"]<<endl;
//     cout<<"============================"<<endl;
// }else if(msgid==CHAT_NOTIFY){
//     cout<<"\n==========私聊消息=========="<<endl;
//     cout<<"来自:"<<js["from"]<<endl;
//     cout<<"消息:"<<js["message"]<<endl;
//     cout<<"============================"<<endl;
// }else if(msgid==GROUP_REQUEST_NOTIFY){
//     cout<<"\n==========加群申请=========="<<endl;
//     cout<<js["message"]<<endl;
//     cout<<"============================"<<endl;
// }else if(msgid==GROUP_OFFLINE_NOTIFY){
//     cout<<"\n\n==========群离线消息=========="<<endl;
//     cout<<js["message"]<<endl;
//     cout<<"============================"<<endl;
// } else if(msgid ==FILE_REQUEST_NOTIFY){
//     cout<<"\n\n==========文件请求=========="<<endl;
//     string msg=js["message"];
//     cout<<msg<<endl;
//     cout<<"============================"<<endl;
// }else{
//     cout<<"other message:"<<result<<endl;
//  }
   }
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

    cout<<"========== RAW RESPONSE =========="<<endl;
    cout<<"len = "<<len<<endl;


    string result=MessageCodec::decode(res);
    
    cout<<"result = "<<result<<endl;
    cout<<"=================================="<<endl;


    json js_=json::parse(result);

    cout<<"response msgid = "
        <<js_["msgid"]
        <<endl;

    cout<<"expected msgid = "
        <<SEND_VERIFY_CODE_ACK
        <<endl;

    cout<<"response = "
        <<js_.dump()
        <<endl;

    if(!js_.contains("msgid")){
    cout<<"invalid verify code response: no msgid"<<endl;
    return false;
    }
    int receivedMsgId = js_["msgid"];
    int expectedMsgId = SEND_VERIFY_CODE_ACK;
    cout<<"receivedMsgId = "<<receivedMsgId<<endl;
    cout<<"expectedMsgId = "<<expectedMsgId<<endl;
    if(receivedMsgId != expectedMsgId){
    cout<<"invalid verify code response"<<endl;
    return false;
}
cout<<"msgid check success"<<endl;
if(js_["errno"] == 0){
    cout<<"send verify code success"<<endl;
    return true;
}
cout<<"send verify code failed:"<<js_["message"]<<endl;
return false;
}
bool registerUser(int fd){
    cout<<"注册"<<endl;
    //1输入
    cout<<"username:"; cin>>username;
    string email;
    cout<<"your email:";
    //cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin>>email;

    //请求验证码
    sendVerifyCode(fd,email);
    //2 输入验证码
    string code;
    cout<<"verify code:";
    //cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin>>code;
    //3 输入密码
    string password;
    cout<<"password:";
    //cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin>>password;

    //2构造注册json语句,发送
    json regMsg;
    regMsg["msgid"]=REGISTER_MSG;
    regMsg["username"]=username;
    regMsg["email"] =email;
    regMsg["code"]=code;
    regMsg["password"]=password;

    string data=MessageCodec::encode(regMsg.dump());
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
    if(result.empty()) cout<<"empty response"<<endl;

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
    string email,code,password;
    cout<<"your email:";cin>>email;
    //请求验证码
    if(!sendVerifyCode(fd,email)){
        cout<<"验证码发送失败"<<endl;
        return false;
    }
    cout<<"your verify code:";cin>>code;
    
    cout<<"your new password:";cin>>password;
    json js;
    js["msgid"]=RESET_PASSWORD_MSG;
    js["email"]=email;
    js["code"]=code;
    js["password"]=password;
    string data=MessageCodec::encode(js.dump());
    send(fd,data.data(),data.size(),0);
    return 1;
}
int main(int argc, char* argv[]){
    string server_ip="10.30.0.128";
    int server_port;
    // 支持：./client IP PORT
    if(argc==3){
        server_ip =argv[1];
        server_port=atoi(argv[2]);
    }else{
        cout << "server port: ";
        cin >> server_port;
    }
    // 1 socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0){
        perror("socket");
        return -1;
    }
    // 2 connect
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);
    if(inet_pton(AF_INET, server_ip.c_str(), &server.sin_addr) <= 0){
        perror("inet_pton");
        return -1;
    }
    if(connect(fd, (sockaddr*)&server, sizeof(server)) < 0){
        perror("connect");
        return -1;
    }
    cout << "connect success" << endl;

    

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
    else if(choice==30){
        if(ResetPassword(fd)) break;
    }

}


    //开线程
    thread t(recvMessage,fd);
    t.detach();

    //timerfd
    //create
    int heartbeatTimerFd=timerfd_create(CLOCK_MONOTONIC,0);
    if(heartbeatTimerFd < 0){
    Logger.instance().error("timerfd_create");
    close(fd);
    return -1;
}
   //设置定时器
   itimerspec timer{};
   //第一次5秒后触发
   timer.it_value.tv_sec = 5;
   //之后每5秒触发一次
   timer.it_interval.tv_sec = 5;
   if(timerfd_settime(heartbeatTimerFd,0,&timer,nullptr)<0){
    Logger.instance().error("timerfd_settime");
    close(heartbeatTimerFd);
    close(fd);
    return -1;
}
cout << "heartbeat timer started, interval=5s" << endl;

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
        //cout<<"29 heartbeat"<<endl;
        cout<<"30 reset password"<<endl;
        cout<<"31 block friend"<<endl;
        cout<<"32 unblock friend"<<endl;
        cout<<"33 send file request"<<endl;
        cout<<"34 accept file request"<<endl;
        //cout<<"37 query file block"<<endl;
        //cout<<"35 send file data"<<endl;
        cout<<"----------------------------------"<<endl;
        
// 打印command:，刷到屏幕
// 擦干净监视名单，登记键盘、闹钟
// select 原地睡觉等待事件
// 情况 A：闹钟时间到 → select 唤醒
// FD_ISSET 判定闹钟就绪
// read 闹钟 fd（关掉闹铃），发送心跳
// 键盘没有事件，continue 回到循环开头，重新打印提示符继续等待
// 情况 B：键盘敲数字回车 → select 唤醒
// FD_ISSET 判定键盘就绪
// cin 读取 cmd 数字，执行业务，回到循环
// 情况 C：来了系统信号打断 select → EINTR，continue 重新等待。

        cout<<"command:";
        cout.flush();//强制把缓冲区内容立刻怼到屏幕上

        //等待用户输入或者心跳定时器
        fd_set readfds;//造一张监视名单，名单上写要监控哪些小文件句柄 (fd) 有没有数据可读
        FD_ZERO(&readfds);//这张名单白纸擦干净，全部清空
        //标准输入
        FD_SET(STDIN_FILENO, &readfds);//告诉 select：帮我盯着键盘，键盘有输入就通知我
        //心跳timerfd
        FD_SET(heartbeatTimerFd, &readfds);//heartbeatTimerFd：timerfd创建出来的闹钟fd-.闹钟响了也要通知我
        //select需要知道最大的fd
        int maxfd = max(STDIN_FILENO, heartbeatTimerFd);//遍历找出我们监控的fd里面数字最大那一个
        int ret = select(maxfd + 1,&readfds,nullptr,nullptr,nullptr);//ret>0:有 1 个或者多个 fd 出事了（有数据来了）
        if(ret < 0){
            if(errno == EINTR) continue;
            Logger.instance().error("select");
            break;
        }
        //心跳timer触发
        if(FD_ISSET(heartbeatTimerFd, &readfds)){//去刚才被 select 修改过的名单上查一查：是不是闹钟 fd 有事件？
            uint64_t exp;
            ssize_t n = read(heartbeatTimerFd,&exp,sizeof(exp));//timerfd 闹钟响了，必须 read 读一下！
            if(n < 0) Logger::instance().error("read timerfd failed");
            else if(n == sizeof(exp))sendHeartbeat(fd);//读成功，调用函数发送心跳包
        }
        //用户输入
        if(!FD_ISSET(STDIN_FILENO, &readfds))continue;

        int cmd; cin>>cmd;
        if(!(cin >> cmd)){
           cout<<"input error"<<endl;
           cin.clear();
           cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
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

            json js;
            js["msgid"]=CHAT_MSG;
            js["from"]=username;
            js["to"]=to;
            js["message"]=msg;

            string sendData=MessageCodec::encode(js.dump());
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

            json js;
            js["msgid"]=GET_PRIVATE_HISTORY;
            js["user1"]=username;
            js["user2"]=friendName;

            string sendData=MessageCodec::encode(js.dump());
            send(fd,sendData.data(),sendData.size(),0);
}

        //查看群聊聊天记录
        else if(cmd==GET_GROUP_HISTORY){
            string groupName;
            cout<<"group name:";cin>>groupName;

            json js;
            js["msgid"]=GET_GROUP_HISTORY;
            js["groupname"]=groupName;

            string sendData=MessageCodec::encode(js.dump());
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

            json js;
            js["msgid"]=KICK_MEMBER_MSG;
            js["groupname"]=groupName;
            js["operator"]=username;
            js["username"]=kick_member;

            string sendData=MessageCodec::encode(js.dump());
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

            json js;
            js["msgid"]=ADD_GROUP_ADMIN_MSG;
            js["groupname"]=groupName;
            js["operator"]=username;
            js["username"]=admin;

            string sendData=MessageCodec::encode(js.dump());
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

            json js;
            js["msgid"]=REMOVE_GROUP_ADMIN_MSG;
            js["groupname"]=groupName;
            js["operator"]=username;
            js["username"]=admin;

            string sendData=MessageCodec::encode(js.dump());
            send(fd,sendData.data(),sendData.size(),0);
}

        //解散群
        else if(cmd==DELETE_GROUP_MSG){
            string groupName;
            cout<<"group name:";
            cin>>groupName;

            json js;
            js["msgid"]=DELETE_GROUP_MSG;
            js["groupname"]=groupName;
            js["operator"]=username;

            string sendData=MessageCodec::encode(js.dump());
            send(fd,sendData.data(),sendData.size(),0);
}

        //查看群聊申请列表
        else if(cmd==GET_GROUP_REQUEST_MSG){
            string groupName;
            cout<<"group name:";
            cin>>groupName;

            json js;
            js["msgid"]=GET_GROUP_REQUEST_MSG;
            js["groupname"]=groupName;
            js["operator"]=username;

            string sendData=MessageCodec::encode(js.dump());
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

            json js;
            js["msgid"]=HANDLE_GROUP_REQUEST_MSG;
            js["groupname"]=groupName;
            js["operator"]=username;
            js["username"]=handle_user;
            js["accept"]=accept;

            string sendData=MessageCodec::encode(js.dump());
            send(fd,sendData.data(),sendData.size(),0);
}

        //请求发送验证码
        else if(cmd==SEND_VERIFY_CODE_MSG){
            string email;
            cout<<"email:";
            cin>>email;

            json js;
            js["msgid"]=SEND_VERIFY_CODE_MSG;
            js["email"]=email;

            string sendData=MessageCodec::encode(js.dump());
            send(fd,sendData.data(),sendData.size(),0);
}

        //心跳检测
        else if(cmd==HEARTBEAT_MSG){
            json js;
            js["msgid"]=HEARTBEAT_MSG;;

            string sendData=MessageCodec::encode(js.dump());
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

            json js;
            js["msgid"]=RESET_PASSWORD_MSG;
            js["email"]=email;
            js["code"]=code;
            js["password"]=password;


            string sendData=MessageCodec::encode(js.dump());
            send(fd,sendData.data(),sendData.size(),0);
}

        //屏蔽好友
        else if(cmd==ADD_BLOCK_MSG){
            string blockname;
            cout<<"block name:";
            cin>>blockname;

            json js;
            js["msgid"]=ADD_BLOCK_MSG;
            js["username"]=username;
            js["blockname"]=blockname;

            string sendData=MessageCodec::encode(js.dump());
            send(fd,sendData.data(),sendData.size(),0);
}

        //取消屏蔽好友
        else if(cmd==REMOVE_BLOCK_MSG){
            string unblockname;
            cout<<"remove block name:";
            cin>>unblockname;
            json js;
            js["msgid"]=REMOVE_BLOCK_MSG;
            js["username"]=username;
            js["blockname"]=unblockname;

            string sendData=MessageCodec::encode(js.dump());
            send(fd,sendData.data(),sendData.size(),0);
}

        //发送文件申请
        else if(cmd==SEND_FILE_REQUEST_MSG){
            int type;
            cout<<"1. send to user"<<endl;
            cout<<"2. send to group"<<endl;
            cout<<"choose:";
            cin>>type;

            string target,filename;
            cout<<"filename:";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin>>filename;
            
            string filepath=FILE_ROOT+filename;
            ifstream file(filepath,ios::binary);
            if(!file.is_open()){
                Logger::instance().error("file not exist");
                cout<<"file not exist"<<endl;
                break;
            }
            file.seekg(0,ios::end);
            long long filesize=file.tellg();
            file.close();
            json js;
            js["msgid"]=SEND_FILE_REQUEST_MSG;
            js["fromname"]=username;
            js["filename"]=filename;
            js["filesize"]=filesize;
            if(type==1){
                cout<<"username:";
                cin>>target;
                js["targetType"]="user";
                js["toname"]=target;
            }else if(type==2){
                cout<<"groupname:";
                cin>>target;
                js["targetType"]="group";
                js["groupname"]=target;
            }else{
                cout<<"invalid type"<<endl;
                break;
            }
            string sendData =MessageCodec::encode(js.dump());
            send(fd,sendData.data(),sendData.size(),0);
}
        //接受文件请求
        else if(cmd==FILE_ACCEPT_MSG){
            string fromname;
            cout<<"accept from who:";cin>>fromname;
            string filename;
            cout<<"filename:";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin>>filename;
            PendingFile file=FileClient::instance().getPendingFile(fromname,filename);
            if(file.filename.empty()){
                cout<<"file request not found"<<endl;
                continue;
            }
            //FileManager::instance().startReceive(fromname,filename,file.filesize);
            json js;
            js["msgid"]=FILE_ACCEPT_MSG;
            js["fromname"]=fromname;

            //js["toname"]=FileClient::instance().getUsername();
            js["filename"]=file.filename;
            //普通好友文件
            if(file.targetType=="user"){
                js["targetType"]="user";
                js["toname"]=FileClient::instance().getUsername();
            }
            //群文件
            else if(file.targetType=="group")
            {
                js["targetType"]="group";
                js["groupname"]=file.groupname;
            }
            
            cout<<"send FILE_ACCEPT_MSG:"<<endl;
            cout<<js.dump(4)<<endl;


            string sendData=MessageCodec::encode(js.dump());
            cout<<"send bytes="<<sendData.size()<<endl;
            send(fd,sendData.data(),sendData.size(),0);     
}            
    }
    close(heartbeatTimerFd);
    close(fd);
    return 0;
}