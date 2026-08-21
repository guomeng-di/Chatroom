#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <sys/timerfd.h>
#include <sys/select.h>
#include <cstdint>
#include <algorithm>
#include <string>
#include <limits>
#include "../netlib/base/Logger.h"
#include "../src/config.h"
#include "ClientMessageHandler/ClientMessageHandler.h"
#include "../protocol/MessageCodec/MessageCodec.h"
#include "../protocol/MsgId.h"
#include "../netlib/net/Buffer/Buffer.h"
#include "menu/FriendMenu/FriendMenu.h"
#include "menu/GroupMenu/GroupMenu.h"
#include "menu/FileMenu/FileMenu.h"
#include "menu/AccountMenu/AccountMenu.h"
#include "FileClient/FileClient.h"
#include "menu/Color.h"
#include <nlohmann/json.hpp>
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
    if(n<=0) Logger::instance().error("[heartbeat] send failed");
    else Logger::instance().info("[heartbeat] send");
}
void recvMessage(int fd){
    char buf[1024*4];
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
    char buf[1024*4];
    int len=recv(fd,buf,sizeof(buf),0);
    if(len<=0){
        cout<<endl<<"server close"<<endl;
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
                cout<<endl<<"login success"<<endl;
                FileClient::instance().setUsername(username);
                return 1; // buffer中剩余消息(含FILE_RESUME_NOTIFY)由recvMessage处理
            }else{
                cout<<endl<<"login fail"<<endl;
                return 0;
            }
        }
        // 登录阶段到达的其他通知，直接分发处理，避免丢失
        ClientMessageHandler::handle(js,fd);
    }
   }
    return 1;
}
bool sendVerifyCode(int fd,const string& email){
    json js;
    js["msgid"]=SEND_VERIFY_CODE_MSG;
    js["email"]=email;
    string data=MessageCodec::encode(js.dump());
    send(fd,data.data(),data.size(),0);
    char buf[1024*4];
    int len=recv(fd,buf,sizeof(buf),0);
    if(len<=0){
        cout<<"server close"<<endl;
        return 0;
    }
    string res(buf,len);
    string result=MessageCodec::decode(res);
    json js_=json::parse(result);
    if(!js_.contains("msgid")){
    cout<<"invalid verify code response: no msgid"<<endl;
    return false;
    }
    int receivedMsgId = js_["msgid"];
    int expectedMsgId = SEND_VERIFY_CODE_ACK;
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
    char buf[1024*4];
    int len=recv(fd,buf,sizeof(buf),0);
    if(len<=0){
        cout<<endl<<COLOR_RED<<"server close"<<COLOR_RESET<<endl;
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
            cout<<endl<<COLOR_RED<<"register fail: "<<js["message"]<<COLOR_RESET<<endl;
            //close(fd);
            return 0;
        }
    }
    cout<<endl<<COLOR_RED<<"unknown response"<<COLOR_RESET<<endl;
    return 0;
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
    int ret=send(fd,data.data(),data.size(),0);
    if(ret<=0){
        cout<<"send reset password failed"<<endl;
        return false;
    }
while(1){
    //等待RESET_PASSWORD_ACK
    char buffer[4096]={0};
    int len=recv(fd,buffer,sizeof(buffer),0);
    if(len<=0){
            cout<<"server close"<<endl;
            close(fd);
            return false;
        }
    clientBuffer.append(buffer,len);
        while(clientBuffer.hasMessage()){
            string msg=clientBuffer.retrieveMessage();
            int msgid=MessageCodec::getMsgId(msg);
            string jsonStr(msg.data()+4,msg.size()-4);
            json response=json::parse(jsonStr);

            if(msgid==RESET_PASSWORD_ACK){
                if(response["errno"]==0){
                    cout<<COLOR_GREEN<<response["message"]<<COLOR_RESET<<endl;
                    return true;
                }else{
                    cout<<COLOR_RED<<response["message"]<<COLOR_RESET <<endl;
                    return false;
                }
            }
            //其他消息继续交给统一处理
            ClientMessageHandler::handle(response,fd);
        }
    }
    return false;
}

void printMainMenu(){
cout<<COLOR_CYAN;
cout<<R"(

==============================
           ChatRoom
==============================
 1. 好友功能
 2. 群聊功能
 3. 文件功能
 4. 账号设置
 0. 退出登录
==============================

)";
cout<<COLOR_RESET;
}
int main(int argc, char* argv[]){
    string server_ip="0.0.0.0";
    int server_port=8888;
    // 支持：./client IP PORT
    if(argc==3){
        server_ip =argv[1];
        server_port=atoi(argv[2]);
    }else{
        // cout<<"server ip:";
        // cin>>server_ip;

        // cout << "server port: ";
        // cin >> server_port;
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
    int ret = inet_pton(AF_INET, server_ip.c_str(), &server.sin_addr);
    if(ret == 0){
      cout<<endl<<"invalid ip address"<<endl;
      return -1;
    }else if(ret < 0){
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
cout << COLOR_GREEN;
cout << R"(
+--------------------------------+
|                                |
|             聊天室              |
+--------------------------------+
|        1. 登录(密码)             |
|        2. 注册                  |
|        3. 重置密码               |
|        0. 退出                  |
+--------------------------------+
)";
cout << COLOR_RESET;

    int choice;
    if(!(cin>>choice)){
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(),'\n');
        cout<<endl<<"输入错误，请输入数字"<<endl;
        continue;
}

    if(choice==1){
        if(login(fd)) break;
    }
    else if(choice==2){
        if(registerUser(fd)&&login(fd)) break;
    }
    else if(choice==3){
        ResetPassword(fd);
    }
    else if(choice==0){
        cout<<COLOR_RED;
        cout<<endl<<"退出客户端"<<endl;
        cout<<COLOR_RESET;

        break;
    }else{
        cout<<COLOR_YELLOW;
        cout<<endl<<"无效选择，请重新输入"<<endl;
        cout<<COLOR_RESET;
        break;
    }

}


    //开线程
    thread t(recvMessage,fd);
    t.detach();

    //timerfd
    //create
    int heartbeatTimerFd=timerfd_create(CLOCK_MONOTONIC,0);
    if(heartbeatTimerFd < 0){
    Logger::instance().error("timerfd_create");
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
    Logger::instance().error("timerfd_settime");
    close(heartbeatTimerFd);
    close(fd);
    return -1;
}
cout << "heartbeat timer started, interval=5s" << endl;

    //循环显示目录
    printMainMenu();
        
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

      while(1){  
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
            Logger::instance().error("select");
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

        int menu;
        if(!(cin>>menu)){
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(),'\n');
            cout<<"输入错误，请输入数字"<<endl;
            continue;
        }
        switch(menu){
            case 1:
            FriendMenu::run(fd,username);
            break;
            
            case 2:
            GroupMenu::run(fd,username);
            break;

            case 3:
            FileMenu::run(fd,username);
            break;

            case 4:
            AccountMenu::run(fd,username);
            break;
            case 0:
            cout<<COLOR_RED;
            cout<<endl<<"退出客户端"<<endl;
            cout<<COLOR_RESET;
            close(fd);
            return 0;
            default:
            cout<<COLOR_YELLOW;
            cout<<endl<<"无效选择，请重新输入"<<endl;
            cout<<COLOR_RESET;

}

        printMainMenu();
    }
                 
    close(heartbeatTimerFd);
    close(fd);
    return 0;
}