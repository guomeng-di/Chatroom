#include "FileMenu.h"
#include "../../../protocol/MessageCodec/MessageCodec.h"
#include "../../../protocol/MsgId.h"
#include "../../FileClient/FileClient.h"
#include "../../../netlib/base/Logger.h"
#include "../../../src/config.h"
#include "../Color.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <limits>
#include <sys/socket.h>
using namespace std;
using json=nlohmann::json;

void FileMenu::run(int fd,const string& username){
while(true){
cout<<COLOR_RESET;
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
//发送文件
else if(cmd==1){
    int type;
    cout<<"发送对象:"<<endl;
    cout<<"1. 用户"<<endl;
    cout<<"2. 群聊"<<endl;
    cout<<"选择:";

    cin>>type;
    string target;
    string filename;
    cout<<"文件名:";
    cin.ignore(numeric_limits<streamsize>::max(),'\n');
    cin>>filename;

    string filepath=FILE_ROOT+filename;
    ifstream file(filepath,ios::binary);
    if(!file.is_open()){
        cout<<"文件不存在"<<endl;
        continue;
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
        cout<<"用户名:";
        cin>>target;

        js["targetType"]="user";
        js["toname"]=target;
    }else if(type==2){
        cout<<"群名称:";
        cin>>target;

        js["targetType"]="group";
        js["groupname"]=target;
    }else{
        cout<<"类型错误"<<endl;
        continue;
    }

    string data=MessageCodec::encode(js.dump());
    send(fd,data.data(),data.size(),0);
    cout<<"文件发送请求成功"<<endl;
}





//接受文件

else if(cmd==2)
{

    string fromname;

    cout<<"发送者:";

    cin>>fromname;



    string filename;


    cout<<"文件名:";

    cin>>filename;



    PendingFile file=
    FileClient::instance()
    .getPendingFile(fromname,filename);



    if(file.filename.empty())
    {

        cout<<"没有找到文件请求"<<endl;

        continue;

    }



    json js;


    js["msgid"]=FILE_ACCEPT_MSG;

    js["fromname"]=fromname;

    js["filename"]=filename;



    //好友文件

    if(file.targetType=="user")
    {

        js["targetType"]="user";

        js["toname"]=username;

    }



    //群文件

    else if(file.targetType=="group")
    {

        js["targetType"]="group";

        js["groupname"]=file.groupname;

    }



    string data=
    MessageCodec::encode(js.dump());


    send(fd,data.data(),data.size(),0);


    cout<<"文件接收请求已发送"<<endl;


}


}


}