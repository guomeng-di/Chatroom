#include "FileMenu.h"
#include "../../../protocol/MessageCodec/MessageCodec.h"
#include "../../../protocol/MsgId.h"
#include "../../FileClient/FileClient.h"
#include "../../Heartbeat/Heartbeat.h"
#include "../../../netlib/base/Logger/Logger.h"
#include "../../../netlib/base/SocketUtil/SocketUtil.h"
#include "../../../src/config.h"
#include "../Color.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <limits>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <cerrno>

using namespace std;
using json=nlohmann::json;

void FileMenu::run(int fd,const string& username){
    while(true){
        cout<<COLOR_BLUE;
        cout<<R"(
+---------------------------+
|        文件管理            |
+---------------------------+
|1. 发送文件                 |
|2. 接收文件                 |
|0. 返回                    |
+---------------------------+
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
            int type;
            cout<<"发送对象:"<<endl;
            cout<<"1. 用户"<<endl;
            cout<<"2. 群聊"<<endl;
            cout<<"选择:";
            cin>>type;

            string target;
            string filepath;

            cout<<"文件路径:";
            cin.ignore(numeric_limits<streamsize>::max(),'\n');
            getline(cin,filepath);

            if(filepath.empty()){
                cout<<"文件路径不能为空"<<endl;
                continue;
            }

            filesystem::path path(filepath);
            string filename=path.filename().string();

            if(filename.empty()){
                cout<<"文件名无效"<<endl;
                continue;
            }

            ifstream file(filepath,ios::binary);
            if(!file.is_open()){
                cout<<"文件不存在"<<endl;
                continue;
            }

            file.seekg(0,ios::end);
            long long filesize=file.tellg();
            file.close();

            if(filesize<0){
                cout<<"无法获取文件大小"<<endl;
                continue;
            }

            json js;
            js["msgid"]=SEND_FILE_REQUEST_MSG;
            js["fromname"]=username;
            js["filename"]=filename;
            js["filesize"]=filesize;
            js["filepath"]=filepath;

            string targetType;

            if(type==1){
                cout<<"用户名:";
                cin>>target;
                if(target.empty()){
                    cout<<"用户名不能为空"<<endl;
                    continue;
                }
                targetType="user";
                js["targetType"]=targetType;
                js["toname"]=target;
            }else if(type==2){
                cout<<"群名称:";
                cin>>target;
                if(target.empty()){
                    cout<<"群名称不能为空"<<endl;
                    continue;
                }
                targetType="group";
                js["targetType"]=targetType;
                js["groupname"]=target;
            }else{
                cout<<"类型错误"<<endl;
                continue;
            }

            FileClient::instance().setSendFilePath(targetType,target,filename,filepath);

            string data=MessageCodec::encode(js.dump());

            if(!SocketUtil::sendAll(fd,data)){
                cout<<"文件发送请求失败"<<endl;
                continue;
            }

            cout<<"文件发送请求已发送"<<endl;
            cout<<"文件名: "<<filename<<endl;
            cout<<"文件大小: "<<filesize<<" bytes"<<endl;
            cout<<"发送对象: "<<target<<endl;
        }

        else if(cmd==2){
            string fromname;
            cout<<"发送者:";
            cin>>fromname;

            string filename;
            cout<<"文件名:";
            cin.ignore(numeric_limits<streamsize>::max(),'\n');
            getline(cin,filename);

            PendingFile file=FileClient::instance().getPendingFile(fromname,filename);

            if(file.filename.empty()){
                cout<<"没有找到文件请求"<<endl;
                continue;
            }

            json js;
            js["msgid"]=FILE_ACCEPT_MSG;
            js["fromname"]=fromname;
            js["filename"]=filename;
            js["fileid"]=file.fileid;

            if(file.targetType=="user"){
                js["targetType"]="user";
                js["toname"]=username;
            }else if(file.targetType=="group"){
                js["targetType"]="group";
                js["groupname"]=file.groupname;
            }

            string data=MessageCodec::encode(js.dump());
            SocketUtil::sendAll(fd,data);

            cout<<"文件接收请求已发送"<<endl;
        }
    }
}