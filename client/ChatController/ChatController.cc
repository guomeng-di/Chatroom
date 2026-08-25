#include "ChatController.h"
#include "../../protocol/MsgId.h"
#include "../../protocol/MessageCodec/MessageCodec.h"
#include "../../netlib/base/SocketUtil/SocketUtil.h"
#include "../menu/Color.h"
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

using namespace std;
using json=nlohmann::json;

class ChatInputMode{
public:
    ChatInputMode():active_(false){
        if(tcgetattr(STDIN_FILENO,&old_)==0){
            termios current=old_;
            current.c_lflag&=~ICANON;
            current.c_cc[VMIN]=1;
            current.c_cc[VTIME]=0;
            if(tcsetattr(STDIN_FILENO,TCSANOW,&current)==0) active_=true;
        }
    }
    ~ChatInputMode(){
        if(active_) tcsetattr(STDIN_FILENO,TCSANOW,&old_);
    }
private:
    termios old_{};
    bool active_;
};

bool readChatLine(string& pending,string& message){
    size_t pos=pending.find('\n');
    if(pos!=string::npos){
        message=pending.substr(0,pos);
        pending.erase(0,pos+1);
        if(!message.empty()&&message.back()=='\r') message.pop_back();
        return true;
    }
    char buf[4096];
    ssize_t len=read(STDIN_FILENO,buf,sizeof(buf));
    if(len<=0) return false;
    pending.append(buf,static_cast<size_t>(len));
    return false;
}

string normalizeChatText(const string& text){
    try{
        json check=json{{"message",text}};
        check.dump();
        return text;
    }catch(const exception&){
        iconv_t converter=iconv_open("UTF-8","GB18030");
        if(converter==reinterpret_cast<iconv_t>(-1)) return text;
        size_t inputSize=text.size();
        size_t outputSize=inputSize*3+1;
        string result(outputSize,'\0');
        char* inputBuffer=const_cast<char*>(text.data());
        char* outputBuffer=result.data();
        size_t remainingOutput=outputSize;
        if(iconv(converter,&inputBuffer,&inputSize,&outputBuffer,&remainingOutput)==static_cast<size_t>(-1)){
            iconv_close(converter);
            return text;
        }
        iconv_close(converter);
        result.resize(outputSize-remainingOutput);
        return result;
    }
}

void ChatController::privateChat(int fd,const string& username){
    string friendName;
    cout<<"好友账号:";
    cin>>friendName;
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
    // cout<<"我: ";
    cout<<COLOR_RESET;

    ChatInputMode inputMode;
    string pendingInput;
    while(true){
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO,&readfds);

        int heartbeatFd=Heartbeat::getTimerFd();
        if(heartbeatFd>=0) FD_SET(heartbeatFd,&readfds);

        int maxfd=STDIN_FILENO;
        if(heartbeatFd>maxfd) maxfd=heartbeatFd;

        bool pendingReady=pendingInput.find('\n')!=string::npos;
        int selectRet=pendingReady?1:select(maxfd+1,&readfds,nullptr,nullptr,nullptr);

        if(selectRet<0){
            if(errno==EINTR) continue;
            cerr<<"select failed"<<endl;
            break;
        }

        if(!pendingReady&&heartbeatFd>=0&&FD_ISSET(heartbeatFd,&readfds)){
            Heartbeat::check(fd);
        }

        if(!pendingReady&&!FD_ISSET(STDIN_FILENO,&readfds)) continue;

        string message;
        if(!readChatLine(pendingInput,message)) continue;

        if(message=="quit"){
            cout<<"退出聊天"<<endl;
            break;
        }

        if(message.empty()){
            //cout<<COLOR_GREEN<<"我: "<<COLOR_RESET;
            continue;
        }

        json js;
        js["msgid"]=CHAT_MSG;
        js["from"]=username;
        js["to"]=friendName;
        js["message"]=normalizeChatText(message);

        string data=MessageCodec::encode(js.dump(-1,' ',false,json::error_handler_t::replace));
        bool sendRet=SocketUtil::sendAll(fd,data);

        if(!sendRet){
            cout<<"发送失败"<<endl;
            break;
        }

        //cout<<COLOR_GREEN<<"我: "<<COLOR_RESET;
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
    //cout<<"我: ";
    cout<<COLOR_RESET;

    ChatInputMode inputMode;
    string pendingInput;
    while(true){
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO,&readfds);

        int heartbeatFd=Heartbeat::getTimerFd();
        if(heartbeatFd>=0) FD_SET(heartbeatFd,&readfds);

        int maxfd=STDIN_FILENO;
        if(heartbeatFd>maxfd) maxfd=heartbeatFd;

        bool pendingReady=pendingInput.find('\n')!=string::npos;
        int selectRet=pendingReady?1:select(maxfd+1,&readfds,nullptr,nullptr,nullptr);

        if(selectRet<0){
            if(errno==EINTR) continue;
            cerr<<"select failed"<<endl;
            break;
        }

        if(!pendingReady&&heartbeatFd>=0&&FD_ISSET(heartbeatFd,&readfds)){
            Heartbeat::check(fd);
        }

        if(!pendingReady&&!FD_ISSET(STDIN_FILENO,&readfds)) continue;

        string message;
        if(!readChatLine(pendingInput,message)) continue;

        if(message=="quit"){
            cout<<"退出聊天"<<endl;
            break;
        }

        if(message.empty()){
            //cout<<COLOR_GREEN<<"我: "<<COLOR_RESET;
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
            cout<<"发送失败"<<endl;
            break;
        }

        //cout<<COLOR_GREEN<<"我: "<<COLOR_RESET;
    }

    cout<<COLOR_BLUE;
    cout<<"已退出群聊 "<<groupName<<endl;
    cout<<COLOR_RESET;
}
