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
#include <deque>

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
        const char* enablePaste="\033[?2004h";
        write(STDOUT_FILENO,enablePaste,8);
    }
    ~ChatInputMode(){
        const char* disablePaste="\033[?2004l";
        write(STDOUT_FILENO,disablePaste,8);
        if(active_) tcsetattr(STDIN_FILENO,TCSANOW,&old_);
    }
private:
    termios old_{};
    bool active_;
};

class ChatInputReader{
public:
    explicit ChatInputReader(bool splitMode):splitMode_(splitMode),inPaste_(false),pendingCr_(false){}

    bool hasMessage() const{
        return !messages_.empty();
    }

    void setSplitMode(bool splitMode){
        splitMode_=splitMode;
    }

    bool isSplitMode() const{
        return splitMode_;
    }

    bool readMessage(string& message){
        if(messages_.empty()){
            char buf[4096];
            ssize_t len=read(STDIN_FILENO,buf,sizeof(buf));
            if(len<=0) return false;
            pending_.append(buf,static_cast<size_t>(len));
            parse();
        }
        if(messages_.empty()) return false;
        message=std::move(messages_.front());
        messages_.pop_front();
        return true;
    }

private:
    void finishLine(){
        messages_.push_back(std::move(current_));
        current_.clear();
    }

    void appendNewline(){
        if(inPaste_&&!splitMode_) current_.push_back('\n');
        else finishLine();
    }

    void appendCharacter(char ch){
        if(ch=='\r'){
            appendNewline();
            pendingCr_=true;
        }else if(ch=='\n'){
            if(pendingCr_){
                pendingCr_=false;
                return;
            }
            appendNewline();
        }else{
            pendingCr_=false;
            current_.push_back(ch);
        }
    }

    bool markerReady(const string& marker) const{
        return pending_.size()>=marker.size()&&pending_.compare(0,marker.size(),marker)==0;
    }

    bool markerIncomplete(const string& marker) const{
        return pending_.size()<marker.size()&&marker.compare(0,pending_.size(),pending_)==0;
    }

    void parse(){
        const string pasteBegin="\033[200~";
        const string pasteEnd="\033[201~";
        while(!pending_.empty()){
            const string& marker=inPaste_?pasteEnd:pasteBegin;
            if(markerReady(marker)){
                pendingCr_=false;
                pending_.erase(0,marker.size());
                if(inPaste_){
                    inPaste_=false;
                    if(splitMode_&&!current_.empty()) finishLine();
                }else{
                    inPaste_=true;
                }
                continue;
            }
            if(markerIncomplete(marker)) return;
            char ch=pending_.front();
            pending_.erase(0,1);
            appendCharacter(ch);
        }
    }

    bool splitMode_;
    bool inPaste_;
    bool pendingCr_;
    string pending_;
    string current_;
    deque<string> messages_;
};

bool handleModeCommand(const string& message,ChatInputReader& inputReader){
    if(message=="/mode 1"){
        inputReader.setSplitMode(false);
        cout<<COLOR_GREEN<<"已切换为整体发送，粘贴文本中的换行和空行会保留"<<COLOR_RESET<<endl;
        return true;
    }
    if(message=="/mode 2"){
        inputReader.setSplitMode(true);
        cout<<COLOR_GREEN<<"已切换为分行发送，粘贴文本会按换行拆成多条消息"<<COLOR_RESET<<endl;
        return true;
    }
    if(message=="/mode"){
        cout<<COLOR_YELLOW;
        cout<<"当前发送方式:"<<(inputReader.isSplitMode()?"分行发送":"整体发送")<<endl;
        cout<<"输入 /mode 1 切换为整体发送"<<endl;
        cout<<"输入 /mode 2 切换为分行发送"<<endl;
        cout<<COLOR_RESET;
        return true;
    }
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
    cout<<"发送方式:整体发送"<<endl;
    cout<<"输入 /mode 查看或切换发送方式"<<endl;
    cout<<"输入 quit 返回"<<endl;
    cout<<COLOR_RESET;
    cout<<COLOR_GREEN;
    // cout<<"我: ";
    cout<<COLOR_RESET;

    ChatInputMode inputMode;
    ChatInputReader inputReader(false);
    while(true){
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO,&readfds);

        int heartbeatFd=Heartbeat::getTimerFd();
        if(heartbeatFd>=0) FD_SET(heartbeatFd,&readfds);

        int maxfd=STDIN_FILENO;
        if(heartbeatFd>maxfd) maxfd=heartbeatFd;

        bool pendingReady=inputReader.hasMessage();
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
        if(!inputReader.readMessage(message)) continue;

        if(message=="quit"){
            cout<<"退出聊天"<<endl;
            break;
        }

        if(handleModeCommand(message,inputReader)) continue;

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
    cout<<"发送方式:整体发送"<<endl;
    cout<<"输入 /mode 查看或切换发送方式"<<endl;
    cout<<"输入 quit 返回"<<endl;
    cout<<COLOR_RESET;
    cout<<COLOR_GREEN;
    //cout<<"我: ";
    cout<<COLOR_RESET;

    ChatInputMode inputMode;
    ChatInputReader inputReader(false);
    while(true){
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO,&readfds);

        int heartbeatFd=Heartbeat::getTimerFd();
        if(heartbeatFd>=0) FD_SET(heartbeatFd,&readfds);

        int maxfd=STDIN_FILENO;
        if(heartbeatFd>maxfd) maxfd=heartbeatFd;

        bool pendingReady=inputReader.hasMessage();
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
        if(!inputReader.readMessage(message)) continue;

        if(message=="quit"){
            cout<<"退出聊天"<<endl;
            break;
        }

        if(handleModeCommand(message,inputReader)) continue;

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
