#include "ClientMessageHandler.h"
#include "../../protocol/MsgId.h"
#include "../FileClient/FileClient.h"
#include "../../netlib/base/SocketUtil/SocketUtil.h"
#include <sys/socket.h>
#include <fstream>
#include <filesystem>
#include "../../manager/FileManager/FileManager.h"
#include <iostream>
#include "../menu/Color.h"
#include "../menu/AccountMenu/AccountMenu.h"
#include <thread>
using namespace std;
void ClientMessageHandler::handle(const json& js,int fd){
        if(!js.contains("msgid")){
            cout<<"invalid message:"<<js<<endl;
        }
        int msgid=js["msgid"];


        //私聊消息
        if(msgid==CHAT_NOTIFY){
            string from=(string)js["from"];
            string message=(string)js["message"];
            cout<<COLOR_CYAN<<from<<COLOR_RESET<<": "<<message<<endl;
        }
        else if(msgid==CHAT_ACK){
            if(js["errno"]!=0){
                //失败显示
            cout<<"\033[31m发送失败:\033[0m "<<js["message"]<<endl;
            }
        }

        //群聊消息
else if(msgid==GROUP_CHAT_NOTIFY){
    string from=(string)js["from"];
    string message=(string)js["message"];

    cout<<COLOR_BLUE<<from<<COLOR_RESET<<": "<<message<<endl;
}else if(msgid==GROUP_CHAT_ACK){
    if(js["errno"]==0) return;
    cout<<COLOR_RED<<"群聊失败: "<<js["message"]<<COLOR_RESET<<endl;
}
//私聊历史
else if(msgid==GET_PRIVATE_HISTORY_ACK){
 cout<<"\n";
 cout<<COLOR_BLUE;
 cout<<"+--------------------------------+"<<endl;
 cout<<"|            私聊历史消息          |"<<endl;
 cout<<"+--------------------------------+"<<endl;
 cout<<COLOR_RESET;

 if(js["errno"]==0){
  if(js["messages"].empty()){
   cout<<COLOR_RED;
   cout<<"没有更多聊天记录"<<endl;
   cout<<COLOR_RESET;
  }else{
   for(auto& msg:js["messages"]){
    string from=msg["from"];
    string message=msg["message"];
    string time=msg["time"];
    string username=FileClient::instance().getUsername();
    cout<<"["<<time<<"] ";
    if(from==username){
     cout<<COLOR_GREEN;
     cout<<"我";
     cout<<COLOR_RESET;
    }else{
     cout<<COLOR_BLUE;
     cout<<from;
     cout<<COLOR_RESET;
    }
    cout<<" : ";
    cout<<message<<endl;
   }

   bool hasMore=js.value("hasMore",false);
   long long nextBeforeId=js.value("nextBeforeId",0LL);

   if(hasMore){
    cout<<COLOR_BLUE;
    cout<<"是否继续查询更早的聊天记录？(1.继续 0.返回):";
    cout<<COLOR_RESET;
    int choice;
    cin>>choice;
    if(choice==1){
     json nextJs;
     nextJs["msgid"]=GET_PRIVATE_HISTORY;
     nextJs["beforeId"]=nextBeforeId;
     nextJs["user1"]=js["user1"];
     nextJs["user2"]=js["user2"];
     string data=MessageCodec::encode(nextJs.dump());
     SocketUtil::sendAll(fd,data);
    }
   }else{
    cout<<COLOR_BLUE;
    cout<<"已经没有更早的聊天记录了"<<endl;
    cout<<COLOR_RESET;
   }
  }
 }else{
  cout<<COLOR_RED;
  cout<<"获取私聊历史失败: "<<js["message"]<<endl;
  cout<<COLOR_RESET;
 }
 cout<<COLOR_BLUE;
 cout<<"+--------------------------------+"<<endl;
 cout<<COLOR_RESET;
}
//群聊历史
else if(msgid==GET_GROUP_HISTORY_ACK){
 cout<<COLOR_BLUE;
 cout<<"+--------------------------------+"<<endl;
 cout<<"|          群聊历史消息          |"<<endl;
 cout<<"+--------------------------------+"<<endl;
 cout<<COLOR_RESET;
 if(js["errno"]==0){
  if(js["messages"].empty()){
   cout<<COLOR_RED;
   cout<<"没有更多聊天记录"<<endl;
   cout<<COLOR_RESET;
  }else{
   for(auto& msg:js["messages"]){
    string from=msg["from"];
    string message=msg["message"];
    string time=msg["time"];
    string username=FileClient::instance().getUsername();
    cout<<"["<<time<<"] ";
    if(from==username){
     cout<<COLOR_GREEN;
     cout<<"我";
     cout<<COLOR_RESET;
    }else{
     cout<<COLOR_BLUE;
     cout<<from;
     cout<<COLOR_RESET;
    }
    cout<<": "<<message<<endl;
   }
   bool hasMore=js.value("hasMore",false);
   long long nextBeforeId=js.value("nextBeforeId",0LL);
   if(hasMore){
    cout<<COLOR_BLUE;
    cout<<"是否继续查询更早的聊天记录？(1.继续 0.返回):";
    cout<<COLOR_RESET;
    int choice;
    cin>>choice;
    if(choice==1){
     json nextJs;
     nextJs["msgid"]=GET_GROUP_HISTORY;
     nextJs["groupname"]=js["groupname"];
     nextJs["beforeId"]=nextBeforeId;
     string data=MessageCodec::encode(nextJs.dump());
     SocketUtil::sendAll(fd,data);
    }
   }else{
    cout<<COLOR_BLUE;
    cout<<"已经没有更早的聊天记录了"<<endl;
    cout<<COLOR_RESET;
   }
  }
 }else{
  cout<<COLOR_RED;
  cout<<"获取群聊历史失败: "<<js["message"]<<endl;
  cout<<COLOR_RESET;
 }
 cout<<COLOR_BLUE;
 cout<<"+--------------------------------+"<<endl;
 cout<<COLOR_RESET;
}
//好友列表
else if(msgid==FRIEND_LIST_ACK){
    cout<<"\n\n==========好友列表=========="<<endl;
    if(js["errno"]==0){
        for(auto& f:js["friends"]){
            cout<<"好友: "<<COLOR_BLUE<<f["name"]<<COLOR_RESET;
            if(f["online"])cout<<"  在线"<<endl;
            else cout<<"  离线"<<endl;
        }
    }else{
        cout<<"获取好友失败:"<<js["message"]<<endl;
    }
    cout<<"============================"<<endl;
}
        
//群列表响应
else if(msgid==GROUP_LIST_ACK){
    cout<<COLOR_YELLOW<<"我的群聊"<<COLOR_RESET<<endl;
    if(js["errno"]==0){
        for(auto& group:js["groups"]){
            cout<<COLOR_GREEN<<group<<COLOR_RESET<<endl;
        }
    }else{
        cout<<COLOR_RED <<"获取群列表失败: "<<js["message"]<<COLOR_RESET <<endl;
    }
}

//好友申请
        else if(msgid==FRIEND_REQUEST_NOTIFY){
            cout<<"\n\n==========好友申请通知=========="<<endl;
            if(js.contains("time"))
               cout<<js["time"]<<endl;
            if(js.contains("fromname"))
               cout<<js["fromname"]<<" 请求添加你为好友"<<endl;
            else
               cout<<js["message"]<<endl;
            cout<<"==============================="<<endl;
        }
else if(msgid==HANDLE_FRIEND_REQUEST_ACK){
    cout<<"\n\n==========好友申请处理=========="<<endl;

    if(js["errno"]==0){
        cout<<COLOR_GREEN<<js["message"]<<COLOR_RESET<<endl;
    }
    else{
        cout<<COLOR_RED<<"失败:"<<js["message"]<<COLOR_RESET<<endl;
    }
    cout<<"================================"<<endl;
}
//好友申请列表
else if(msgid==GET_FRIEND_REQUEST_ACK){
    cout<<COLOR_BLUE;
    cout<<"\n\n+--------------------------------+\n";
    cout<<"|            好友申请列表          |\n";
    cout<<"+--------------------------------+\n";
    cout<<COLOR_RESET;

    if(js["errno"]==0){
        if(js["requests"].empty()){
            cout<<"暂无好友申请"<<endl;
        }else{
            for(auto& request:js["requests"]){
                cout<<COLOR_GREEN;
                cout<<"申请人: "<<request["fromname"]<<endl;
                cout<<COLOR_RESET;
                cout<<"申请时间: "<<request["time"]<<endl;
                cout<<"--------------------------------"<<endl;
            }
        }
    }else{
        cout<<COLOR_RED;
        cout<<"获取好友申请失败: " <<js["message"]<<endl;
        cout<<COLOR_RESET;
    }
    cout<<COLOR_BLUE;
    cout<<"+--------------------------------+\n";
    cout<<COLOR_RESET;
}
//删除好友响应
else if(msgid==DELETE_FRIEND_ACK){
    cout<<COLOR_BLUE;
    cout<<"\n\n+--------------------------------+\n";
    cout<<"|            删除好友            |\n";
    cout<<"+--------------------------------+\n";
    cout<<COLOR_RESET;

    if(js["errno"]==0){
        cout<<COLOR_GREEN;
        cout<<"删除好友成功";
        cout<<COLOR_RESET<<endl;
    }else{
        cout<<COLOR_RED;
        cout<<"删除好友失败: "<<js["message"];
        cout<<COLOR_RESET<<endl;
    }
    cout<<COLOR_BLUE;
    cout<<"+--------------------------------+\n";
    cout<<COLOR_RESET;
}
//离线消息
        else if(msgid == OFFLINE_MSG){
            cout<<"\n\n==========离线消息=========="<<endl;
            try{
                json offline = json::parse((string)js["message"]);
                string from = offline["from"];
                string message = offline["message"];
                cout<<COLOR_GREEN<<from<<COLOR_RESET<<": "<<message<<endl;

                //客户端先发,再出现"我:"
                cout<<COLOR_GREEN;
                //cout<<"我: ";
                cout<<COLOR_RESET;

            }catch(exception& e){
                cout<<endl<<"离线消息解析失败"<<e.what()<<endl;
            }
            cout<<"============================"<<endl;
}
//群离线消息
        else if(msgid==GROUP_OFFLINE_NOTIFY){
            cout<<"\n\n==========群离线消息=========="<<endl;
            try{
                json offline = json::parse((string)js["message"]);
                string from = offline["from"];
                string message = offline["message"];
                cout<<COLOR_GREEN<<from<<COLOR_RESET<<": "<<message<<endl;

                //客户端先发,再出现"我:"
                cout<<COLOR_GREEN;
                cout<<"我: ";
                cout<<COLOR_RESET;
            }catch(exception& e){
                cout<<endl<<"群离线消息解析失败"<<e.what()<<endl;
            }
            cout<<"============================"<<endl;
}
//发送好友申请响应
else if(msgid==SEND_FRIEND_REQUEST_ACK){
    cout<<"\n\n";
    cout<<COLOR_BLUE;
    cout<<"+--------------------------------+\n";
    cout<<"|            好友申请            |\n";
    cout<<"+--------------------------------+\n";
    cout<<COLOR_RESET;

    if(js["errno"]==0){
        cout<<COLOR_GREEN;
        cout<<"好友申请发送成功"<<endl;
        cout<<COLOR_RESET;
    }else{
        cout<<COLOR_RED;
        cout<<"好友申请失败: "<<js["message"].get<string>()<<endl;
        cout<<COLOR_RESET;
    }
    cout<<COLOR_BLUE;
    cout<<"+--------------------------------+\n";
    cout<<COLOR_RESET;
}
//在线状态变化
        else if(msgid==FRIEND_STATUS_NOTIFY){
            cout<<"\n\n==========好友状态变化=========="<<endl;
            cout<<js["username"];
            if(js["online"]) cout<<" 上线"<<endl;
            else cout<<" 下线"<<endl;
            cout<<"================================"<<endl;
        } 
//踢人
else if(msgid==KICK_MEMBER_ACK){
    if(js["errno"]==0){
        cout<<COLOR_GREEN<<js["message"]<<COLOR_RESET<<endl;
    }else{
        cout<<COLOR_RED<<"踢人失败: "<<js["message"]<<COLOR_RESET<<endl;
    }
}
//添加管理员
else if(msgid==ADD_GROUP_ADMIN_ACK){
    if(js["errno"]==0){
        cout<<COLOR_GREEN<<js["message"]<<COLOR_RESET<<endl;
    }else{
        cout<<COLOR_RED<<"添加管理员失败: "<<js["message"]<<COLOR_RESET<<endl;
    }
}
//删除管理员
else if(msgid==REMOVE_GROUP_ADMIN_ACK){
    if(js["errno"]==0){
        cout<<COLOR_GREEN<<js["message"]<<COLOR_RESET<<endl;
    }else{
        cout<<COLOR_RED<<"删除管理员失败: "<<js["message"]<<COLOR_RESET<<endl;
    }
}
//创建群响应
else if(msgid==CREATE_GROUP_ACK){
    if(js["errno"]==0){
        cout<< COLOR_GREEN<< js["message"]<< COLOR_RESET<< endl;
    }else{
        cout<< COLOR_RED<< js["message"]<< COLOR_RESET<< endl;
    }
}
//申请加入群聊响应
else if(msgid==JOIN_GROUP_ACK){
    if(js["errno"]==0){
        cout<< COLOR_GREEN<< js["message"]<< COLOR_RESET<< endl;
    }else{
        cout<< COLOR_RED<< js["message"]<< COLOR_RESET<< endl;
    }
}
//解散群聊
else if(msgid==DELETE_GROUP_ACK){
    if(js["errno"]==0){
        cout<<COLOR_GREEN<<js["message"]<<COLOR_RESET<<endl;
    }else{
        cout<<COLOR_RED<<"解散群聊失败: "<<js["message"]<<COLOR_RESET<<endl;
    }
}
//群聊申请
        else if(msgid==GROUP_REQUEST_NOTIFY){
            cout<<"\n\n==========群申请通知=========="<<endl;
            cout<<"group: "<<js["groupname"]<<endl;
            cout<<js["message"]<<endl;
            cout<<"==============================="<<endl;
        }
//申请加入群聊的人员列表
else if(msgid==GET_GROUP_REQUEST_ACK){
    cout<<COLOR_YELLOW<<"群申请列表"<<COLOR_RESET<<endl;
    if(js["errno"]==0){
        for(auto& request:js["requests"]){
            cout<<COLOR_BLUE<<"username: "<<request["username"]<<COLOR_RESET<<endl;
            cout<<"time: "<<request["time"]<<endl;
        }
    }else{
        cout<<COLOR_RED<<"获取群申请列表失败: "<<js["message"]<<COLOR_RESET<<endl;
    }
}
//处理群申请
else if(msgid==HANDLE_GROUP_REQUEST_ACK){
 cout<<COLOR_RED<<"[群申请处理] "<<js["message"]<<COLOR_RESET<<endl;
}
//心跳检测
        else if(msgid == HEARTBEAT_ACK){
            //  cout<<"\n==========心跳检测=========="<<endl;
            //  cout<<"heartbeat success"<<endl;
            //  cout<<"============================"<<endl;

        }
//退群通知
else if(msgid == GROUP_LEAVE_NOTIFY){
    cout<<COLOR_BLUE<<js["message"]<<COLOR_RESET<<endl;

}
//退出群响应
else if(msgid==LEAVE_GROUP_ACK){
    if(js["errno"]==0){
        cout<< COLOR_GREEN<< js["message"]<< COLOR_RESET << endl;
    }else{
        cout<< COLOR_RED<< js["message"] << COLOR_RESET<< endl;
    }
}
//邀请进群
else if(msgid==GROUP_INVITE_NOTIFY){
    cout<<COLOR_BLUE<<"========== 群邀请 =========="<<COLOR_RESET<<endl;
    cout<<COLOR_BLUE<<"邀请人: "<<js.value("operator","")<<COLOR_RESET<<endl;
    cout<<COLOR_BLUE<<"群名称: "<<js.value("groupname","")<<COLOR_RESET<<endl;
    cout<<COLOR_BLUE<<"消息: "<<js.value("message","")<<COLOR_RESET<<endl;
    cout<<COLOR_BLUE<<"============================"<<COLOR_RESET<<endl;
}
else if(msgid==GROUP_MEMBER_JOIN_NOTIFY){
    cout<<COLOR_BLUE<<js["message"]<<COLOR_RESET<<endl;
}
//查看群成员响应
else if(msgid==GROUP_MEMBER_ACK){
    if(js["errno"]==0){
        cout<< COLOR_GREEN<< js["message"]<< COLOR_RESET<< endl;
        for(auto& member:js["members"]){
            cout<< COLOR_BLUE<< member<< COLOR_RESET<< endl;
        }
    }else{
        cout<< COLOR_RED<< js["message"]<< COLOR_RESET<< endl;
    }
}
//邀请进群响应
else if(msgid==INVITE_GROUP_ACK){
    if(js.value("errno",1)==0){
        cout<< COLOR_GREEN<< js.value("message","invite success")<< COLOR_RESET<< endl;
    }else{
        cout<< COLOR_RED<< js.value("message","invite failed")<< COLOR_RESET<< endl;
    }
}

//屏蔽成功
else if(msgid==ADD_BLOCK_ACK){
    if(js["errno"]==0){
        cout<<COLOR_GREEN<<js["message"]<<COLOR_RESET<<endl;
    }else{
        cout<<COLOR_RED <<"屏蔽好友失败: "<<js["message"]<<COLOR_RESET<<endl;
    }
}

//取消屏蔽成功
else if(msgid==REMOVE_BLOCK_ACK){
    if(js["errno"]==0){
        cout<<COLOR_GREEN<<js["message"]<<COLOR_RESET<<endl;
    }else{
        cout<<COLOR_RED<<"取消屏蔽失败: "<<js["message"]<<COLOR_RESET <<endl;
    }
}
        //发送文件申请
        else if(msgid==FILE_REQUEST_NOTIFY){
            cout<<"\n\n==========文件申请=========="<<endl;
            if(!js.contains("fromname") ||!js.contains("filename") ||!js.contains("filesize")){
                cout<<"file request info error"<<endl;
                return;
            }
            string fromname=js["fromname"];
            string filename=js["filename"];
            long long filesize=js["filesize"];

            cout<<"fromname: "<<js["fromname"]<<endl;
            cout<<"filename: "<<js["filename"]<<endl;
            cout<<"filesize: "<<js["filesize"]<<endl;

            PendingFile file;
            file.sender=fromname;
            file.filename=filename;
            file.filesize=filesize;
            if(js.contains("fileid")) file.fileid=js["fileid"];

            //判断普通文件还是群文件
            if(js.contains("targetType")&&js["targetType"]=="group"){
                file.targetType="group";
                file.groupname=js["groupname"];
            }else file.targetType="user";


            // 保存待接收文件请求
            FileClient::instance().addPendingFile(file.sender,file.filename,file.filesize,file.targetType,file.groupname,file.fileid);
            cout<<"pending file saved"<<endl;
            cout<<"============================"<<endl;
        }

        //接收文件发送请求(收到 FILE_ACCEPT_NOTIFY 的人是文件发送者)
       else if(msgid == FILE_ACCEPT_NOTIFY){
        cout << "\n==========文件请求通过=========="<< endl;
        if(!js.contains("filename") ||!js.contains("receiver") ||!js.contains("fileid")){
        cout << "file accept info missing"<< endl;
        return;
    }
    string receiver=js["receiver"];
    string filename = js["filename"];
    int fileid = js["fileid"];

    string targetType="user";
    string groupname="";
    if(js.contains("targetType")) targetType=js["targetType"];
    if(targetType=="group" && js.contains("groupname")) groupname=js["groupname"];
    string targetKey = (targetType == "group") ? groupname : receiver;
    if(js.contains("filepath") && js["filepath"].is_string()){
        FileClient::instance().setSendFilePath(
            targetType, targetKey, filename, js["filepath"].get<string>());
    }
    string filepath = FileClient::instance().getSendFilePath(targetType,targetKey,filename);
    if(filepath.empty()){
        cout << "file path not found for " << filename << endl;
        return;
    }
    long long offset=0;
    if(js.contains("received_size")) offset=js["received_size"];
    cout<<"start send file"<<endl;
     FileClient::instance().sendFile(fd,fileid,filepath,filename,receiver,targetType,groupname,offset);
}


        //文件请求失败(输入错误名字..),返回结果给发送方
        else if(msgid==FILE_ACCEPT_ACK){
            cout<<"==========文件接收结果=========="<<endl;
            cout<<js["message"]<<endl;
            cout<<"==============================="<<endl;
}


        //发送完成
        else if(msgid==FILE_FINISH_NOTIFY){
            cout<<"===================="<<endl;
            if(js.contains("filename")) cout<<"filename:"<<js["filename"]<<endl;
            if(js.contains("fromname")) cout<<"from:"<<js["fromname"]<<endl;
            if(js.contains("message")) cout<<js["message"]<<endl;
            cout<<"===================="<<endl;
}
else if(msgid==FILE_RESUME_REPLY){
    cout<<"==========断点查询结果=========="<<endl;
    if(js.contains("errno") && js["errno"]!=0){
        cout<<"query resume failed: "<<js.value("message","")<<endl;
    }else{
        cout<<"received_size="<<js.value("received_size",0LL)<<endl;
        cout<<"filename="<<js.value("filename","")<<endl;
        cout<<"fileid="<<js.value("fileid",-1)<<endl;
    }
    cout<<"=============================="<<endl;
}
else if(msgid==FILE_RESUME_NOTIFY){
    cout<<"发现未完成文件"<<endl;
    json query;
    query["msgid"]=FILE_RESUME_REQUEST;
    query["sender"]=js["fromname"];
    query["receiver"]=FileClient::instance().getUsername();
    query["filename"]=js["filename"];
    if(js.contains("fileid")) query["fileid"] = js["fileid"];
    string data=MessageCodec::encode(query.dump());
    SocketUtil::sendAll(fd,data);
}
        //离线后上线文件恢复
        else if(msgid==FILE_RESUME_SEND){
            cout<<"==========恢复发送文件=========="<<endl;
            if(!js.contains("fileid") ||!js.contains("filename") ||!js.contains("receiver") ||!js.contains("received_size")){
                cout << "resume info missing" << endl;
                return;
            }
            int fileid=js["fileid"];
            string filename=js["filename"];
            string receiver=js["receiver"];
            long long offset=js["received_size"];

            cout << "fileid=" << fileid << endl;
            cout << "filename=" << filename << endl;
            cout << "receiver=" << receiver << endl;
            cout<<"already receive size="<<offset<<endl;
            cout<<"开始断点续传..."<<endl;
            string targetType = js.value("targetType","user");
            string groupname = js.value("groupname","");
            string targetKey = (targetType=="group") ? groupname : receiver;
            if(js.contains("filepath") && js["filepath"].is_string()){
                FileClient::instance().setSendFilePath(
                    targetType, targetKey, filename, js["filepath"].get<string>());
            }
            string filepath = FileClient::instance().getSendFilePath(targetType,targetKey,filename);
            if(filepath.empty()){
                cout<<"没有找到待发送文件路径"<<endl;
                return;
            }
            FileClient::instance().sendFile(fd,fileid,filepath,filename,receiver,targetType,groupname,offset);
            cout << "============================" << endl;
        }

        //退出登录
else if(msgid==LOGOUT_ACK){
    if(js["errno"]==0){
        cout<<COLOR_GREEN;
        cout<<"退出登录成功"<<endl;
        cout<<COLOR_RESET;
    }else{
        cout<<COLOR_RED;
        cout<<"退出登录失败: "<<js["message"]<<endl;
        cout<<COLOR_RESET;
    }
}
//发送验证码响应
else if(msgid==SEND_VERIFY_CODE_ACK){
      AccountMenu::setVerifyCodeResult(js.value("errno",1)==0);
      if(js["errno"]==0){
        cout<<COLOR_GREEN;
        cout<<"验证码发送成功"<<endl;
        cout<<COLOR_RESET;

    }else{
        cout<<COLOR_RED;
        cout<<"验证码发送失败: "<<js["message"]<<endl;
        cout<<COLOR_RESET;
    }
}

//注销账号
else if(msgid==DELETE_ACCOUNT_ACK){
    cout<<"msgid: "<<msgid<<endl;
    if(js["errno"]==0){
        cout<<COLOR_GREEN;
        cout<<"注销账号成功"<<endl;
        cout<<COLOR_RESET;
    }else{
        cout<<COLOR_RED;
        cout<<"注销账号失败: "<<js["message"]<<endl;
        cout<<COLOR_RESET;
    }
}
//注册
else if(msgid==REGISTER_ACK){
    cout<<"msgid: "<<msgid<<endl;
    if(js["errno"]==0){
        cout<<COLOR_GREEN;
        cout<<"注册成功"<<endl;
        cout<<COLOR_RESET;
    }else{
        cout<<COLOR_RED;
        cout<<"注册失败: "<<js["message"]<<endl;
        cout<<COLOR_RESET;
    }
}
//登录
else if(msgid==LOGIN_ACK){
    if(js["errno"]==0){
        cout<<COLOR_GREEN;
        cout<<"登录成功"<<endl;
        cout<<COLOR_RESET;
    }else{
        cout<<COLOR_RED;
        cout<<"登录失败: "<<js["message"]<<endl;
        cout<<COLOR_RESET;
    }
}
//重置密码验证码校验响应
else if(msgid==RESET_PASSWORD_ACK){
    if(js["errno"]==0){
        cout<<COLOR_GREEN;
        cout<<"密码修改成功"<<endl;
        cout<<COLOR_RESET;

    }else{
        cout<<COLOR_RED;
        cout<<"密码修改失败: "<<js["message"]<<endl;
        cout<<COLOR_RESET;
    }
}

        //文件重发同意
        else if(msgid==FILE_RESUME_ACCEPT){
            cout<<"\n\n==========请求重发文件=========="<<endl;
            cout<<js.dump(4)<<endl;
            if(!js.contains("fileid") ||!js.contains("filename") ||!js.contains("sender") ||!js.contains("received_size")){
                cout<<"resume info missing"<<endl;
                return;
            }

            int fileid=js["fileid"];
            string filename=js["filename"];
            string sender =js["sender"];
            string receiver=js["receiver"];
            long long received_size=js["received_size"];

            cout<<"fileid="<<fileid<<endl;
            cout<<"filename="<<filename<<endl;
            cout<<"sender="<<sender<<endl;
            cout<<"receiver="<<receiver<<endl;
            cout<<"already received size="<<received_size<<endl;
            cout<<"等待发送方继续发送..."<<endl;
            cout<<"============================"<<endl;
}  
   
        //文件分片 ACK
//         else if(msgid == FILE_BLOCK_ACK){
//             if(!js.contains("fileid") ||!js.contains("blockid")){
//                 cout << "FILE_BLOCK_ACK lack params" << endl;
//                 return;
//             }
//             int fileid = js["fileid"];
//             int blockid = js["blockid"];

//             cout<<"========== FILE BLOCK ACK =========="<<endl;
//             cout<<"fileid="<<fileid<<endl;
//             cout<<"blockid="<<blockid<<endl;
//             cout<<"===================================="<<endl;
//             return;
// }
else if(msgid ==SEND_FILE_REQUEST_ACK){
    if(js["errno"] == 0){
        cout << COLOR_GREEN<< "[好友请求] "<< js["message"].get<string>()<< COLOR_RESET<< endl;
    }else{
        cout << COLOR_RED<< "[好友请求失败] "
             << js["message"].get<string>()
             << COLOR_RESET
             << endl;
    }
}
        //普通响应
        else{
            cout<<"\n\n==========收到消息=========="<<endl;
            cout<<js<<endl;
            cout<<"============================"<<endl;
        }  

    }
