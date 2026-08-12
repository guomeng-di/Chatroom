#include "ClientMessageHandler.h"
#include "../../protocol/MsgId.h"
#include "../FileClient/FileClient.h"
#include <iostream>
using namespace std;
void ClientMessageHandler::handle(const json& js,int fd){
        if(!js.contains("msgid")){
            cout<<"invalid message:"<<js<<endl;
        }
        int msgid=js["msgid"];

        //私聊消息
        if(msgid==CHAT_NOTIFY){
            cout<<"\n\n==========收到私聊=========="<<endl;
            cout<<js["from"] <<": "<<js["message"]<<endl;
            cout<<"============================"<<endl;
        }
        //私聊历史
        else if(msgid==GET_PRIVATE_HISTORY_ACK){
            cout<<"\n\n=========私聊历史消息========"<<endl;
            if(js["errno"]==0){
                for(auto& msg:js["messages"])
                  cout<<msg["time"]<<" "<<msg["from"]<<" : "<<msg["message"]<<endl;
           }else  cout<<"get private history failed:"<<js["message"]<<endl;
           cout<<"==============================="<<endl;
        }
        //群聊历史
        else if(msgid==GET_GROUP_HISTORY_ACK){
            cout<<"\n\n=========群聊历史消息========"<<endl;
            if(js["errno"]==0){
                for(auto& msg:js["messages"])
                  cout<<msg["time"]<<" "<<msg["from"]<<" : "<<msg["message"]<<endl;
           }else  cout<<"get group history failed:"<<js["message"]<<endl;
           cout<<"==============================="<<endl;
        }
        //群聊消息
        else if(msgid==GROUP_CHAT_NOTIFY){
            cout<<"\n\n==========收到群消息=========="<<endl;
            cout<<"群:"<<js["groupname"]<<endl;
            cout<<js["from"]<<": "<<js["message"]<<endl;
            cout<<"=============================="<<endl;
        }
        //群列表响应
        else if(msgid==GROUP_LIST_ACK){
            cout<<"\n\n==========我的群聊=========="<<endl;
            if(js["errno"]==0){
               for(auto& group:js["groups"])
                   cout<<"group: "<<group<<endl;
                }else cout<<"get group list failed:"<<js["message"]<<endl;
            cout<<"============================"<<endl;
        }
        //好友申请通知
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
        //离线消息
        else if(msgid == OFFLINE_MSG){
            cout<<"\n\n==========离线消息=========="<<endl;
            cout<<js["message"]<<endl;
            cout<<"============================"<<endl;
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
            cout<<"\n=================踢人结果=============="<<endl;
            cout<<js["message"]<<endl;
            cout<<"========================================"<<endl;
        }
        //添加管理员
        else if(msgid==ADD_GROUP_ADMIN_ACK){
            cout<<"\n===============管理员设置==============="<<endl;
            cout<<js["message"]<<endl;
            cout<<"========================================"<<endl;
        }
        //删除管理员
        else if(msgid==REMOVE_GROUP_ADMIN_ACK){
            cout<<"\n============删除管理员============"<<endl;
            cout<<js["message"]<<endl;
            cout<<"=================================="<<endl;
        }
        //解散群聊
        else if(msgid==DELETE_GROUP_ACK){
            cout<<"\n============解散群聊============"<<endl;
            cout<<js["message"]<<endl;
            cout<<"================================="<<endl;
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
            cout<<"\n\n==========群申请列表=========="<<endl;
            if(js["errno"]==0){
               for(auto& request:js["requests"]){
                   cout<<"username: "<<request["username"]<<endl;
                   cout<<"time:"<<request["time"]<<endl;
                   cout<<"-----------------------"<<endl;
                }
            }else cout<<"get group list failed:"<<js["message"]<<endl;
            cout<<"==============================="<<endl;
        
        }  
        //请求验证码
        else if(msgid==SEND_VERIFY_CODE_ACK){
             cout<<"\n==========验证码=========="<<endl;
             cout<<js["message"]<<endl;
             cout<<"============================"<<endl;

}
        //心跳检测
        else if(msgid==HEARTBEAT_ACK){
             cout<<"\n==========心跳检测=========="<<endl;
             cout<<"heartbeat success"<<endl;
             cout<<"============================"<<endl;

}
        //重置密码
        else if(msgid==RESET_PASSWORD_ACK){
             if(js["errno"]==0){
                cout<<"\n==========密码设置=========="<<endl;
                cout<<"reset password success"<<endl;
                cout<<"============================="<<endl;
            }else  cout<<"reset password failed:"<<js["message"]<<endl;
}
        //屏蔽成功
        else if(msgid==ADD_BLOCK_ACK){
            cout<<"\n\n==========屏蔽好友=========="<<endl;
            cout<<js["message"]<<endl;
            cout<<"============================"<<endl;
}
        //取消屏蔽成功
        else if(msgid==REMOVE_BLOCK_ACK){
            cout<<"\n\n==========取消屏蔽=========="<<endl;
            cout<<js["message"]<<endl;
            cout<<"============================"<<endl;
}
        //发送文件申请
        else if(msgid==FILE_REQUEST_NOTIFY){
            cout<<"\n\n==========文件请求=========="<<endl;
            //message里存文件信息
            json fileInfo;
            if(js.contains("message") &&js["message"].is_string()){
                //离线消息
                fileInfo=json::parse(js["message"].get<string>());
            }else{
                //在线消息
                fileInfo=js;
            }

            string fromname=fileInfo["fromname"];
            string filename=fileInfo["filename"];
            long long filesize=fileInfo["filesize"];
        
            if(!fileInfo.contains("fromname")||!fileInfo.contains("filename")||!fileInfo.contains("filesize")){
                cout<<"file request info error"<<endl;
                return;
            }
            cout<<"发送者:"<<fromname<<endl;
            cout<<"文件:"<<filename<<endl;
            cout<<"大小:"<<filesize<<endl;

            FileClient::instance().setPendingFile(fromname,filename,filesize);
            cout<<"============================"<<endl;
        }
        //接收文件发送请求(收到 FILE_ACCEPT_NOTIFY 的人是文件发送者)
        else if(msgid==FILE_ACCEPT_NOTIFY){
            cout<<"\n==========文件请求通过=========="<<endl;
            if(!js.contains("filename")||!js.contains("fromname")){
                cout<<"file accept info missing"<<endl;
                return;
            }
            string sender=js["fromname"];
            string filename=js["filename"];

            cout<<sender<<" 接受文件"<<endl;
            cout<<"开始发送文件:"<<filename<<endl;
            
            FileClient client;
            client.sendFile(fd,filename,sender);
            cout<<"============================"<<endl;
        }
         
        //普通响应
        else{
            cout<<"\n\n==========收到消息=========="<<endl;
            cout<<js<<endl;
            cout<<"============================"<<endl;
        }  

}