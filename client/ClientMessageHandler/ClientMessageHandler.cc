#include "ClientMessageHandler.h"
#include "../../protocol/MsgId.h"
#include "../FileClient/FileClient.h"
#include <sys/socket.h>
#include <fstream>
#include <filesystem>
#include "../../manager/FileManager/FileManager.h"
#include <iostream>
#include <thread>
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
        //群离线消息
        else if(msgid==GROUP_OFFLINE_NOTIFY){
    cout<<"\n\n==========群离线消息=========="<<endl;
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
            //  cout<<"\n==========心跳检测=========="<<endl;
            //  cout<<"heartbeat success"<<endl;
            //  cout<<"============================"<<endl;

        }
        //退群成功
        else if(msgid == GROUP_LEAVE_NOTIFY){
            cout << "\n==========群成员退出==========" << endl;
            cout << js["message"] << endl;
            cout << "==============================" << endl;
}
        //重置密码
        else if(msgid==RESET_PASSWORD_ACK){
             if(js["errno"]==0){
                cout<<"\n==========密码设置=========="<<endl;
                cout<<"reset password success"<<endl;
                cout<<"============================="<<endl;
            }else{
                cout<<"\n==========密码设置失败=========="<<endl;
                cout<<"reset password failed:"<<js["message"]<<endl;
                cout<<"============================="<<endl;
            }
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

            //判断普通文件还是群文件
            if(js.contains("targetType")&&js["targetType"]=="group"){
                file.targetType="group";
                file.groupname=js["groupname"];
            }else file.targetType="user";


            // 保存待接收文件请求
            FileClient::instance().addPendingFile(file.sender,file.filename,file.filesize,file.targetType,file.groupname);
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
    // string receiver = js["fromname"];
    string sender =FileClient::instance().getUsername();
    string filename = js["filename"];
    int fileid = js["fileid"];

    vector<int> blocks;
    if(js.contains("blocks")){
        for(auto& b:js["blocks"]){
            blocks.push_back(b);
            cout<<"exist block: "<<b<<endl;
        }
    }
    cout<<"start send file"<<endl;
    string target=receiver;
    string targetType="user";
    string groupname="";
    if(js.contains("targetType")) targetType=js["targetType"];
    if(targetType=="group") groupname=js["groupname"];
     FileClient::instance().sendFile(fd,fileid,filename,target,targetType,groupname,blocks);
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


//         //离线后重新登录
//         else if(msgid==FILE_RESUME_NOTIFY){
//             cout<<"\n==========未完成文件=========="<<endl;
//             string sender=js["fromname"];
//             string filename=js["filename"];
//             long long filesize=js["filesize"];

//             cout<<"发送者:"<<sender<<endl;
//             cout<<"文件:"<<filename<<endl;
//             cout<<"大小:"<<filesize<<endl;
            
//             cout<<"是否继续接收?"<<endl;
//             cout<<"1.继续"<<endl;
//             cout<<"2.放弃"<<endl;
            
//             int choice;cin>>choice;
//             if(choice==1){
//                 json request;
//                 request["msgid"]=QUERY_SEND_FILE_BLOCK_MSG;
//                 request["sender"]=sender;
//                 request["filename"]=filename;
//                 request["receiver"]=FileClient::instance().getUsername();
                
//                 string data=MessageCodec::encode(request.dump());
//                 send(fd,data.data(),data.size(),0);
//             }
            
//             cout<<"============================"<<endl;
// }

else if(msgid==FILE_RESUME_NOTIFY){
    cout<<"发现未完成文件"<<endl;
    json query;
    query["msgid"]=QUERY_SEND_FILE_BLOCK_MSG;
    query["sender"]=js["fromname"];
    query["receiver"]=FileClient::instance().getUsername();
    query["filename"]=js["filename"];
    string data=MessageCodec::encode(query.dump());
    send(fd,data.data(),data.size(),0);
}
        //离线后上线文件恢复
        else if(msgid==FILE_RESUME_SEND){
            cout<<"==========恢复发送文件=========="<<endl;
            if(!js.contains("fileid") ||!js.contains("filename") ||!js.contains("receiver") ||!js.contains("blocks")){
                cout << "resume info missing" << endl;
                return;
            }
            int fileid=js["fileid"];
            string filename=js["filename"];
            string receiver=js["receiver"];
            
            vector<int> blocks;
            for(auto& b:js["blocks"]) blocks.push_back(b);

            cout << "fileid=" << fileid << endl;
            cout << "filename=" << filename << endl;
            cout << "receiver=" << receiver << endl;

            cout<<"已经收到:"<<blocks.size()<<" blocks"<<endl;
            cout << "blocks:";
            
            for(int block : blocks) cout << block << " ";
            
            cout << endl<<"开始断点续传..." << endl;
            //不能直接调用sendFile(),因为当前线程负责接收ACK
            //sendFile()会waitForBlock阻塞当前线程
            std::thread sendThread([&fd,fileid,filename,receiver,blocks](){
                FileClient::instance().sendFile(fd,fileid,filename,receiver,"user","",blocks);
            }
        );
            sendThread.detach();
            // FileClient::instance().sendFile(fd,fileid,filename,receiver,"user","",blocks);
            cout << "============================" << endl;
        }

        //文件重发同意
        else if(msgid==FILE_RESUME_ACCEPT){
            cout<<"\n\n==========请求重发文件=========="<<endl;
            cout<<js.dump(4)<<endl;
            if(!js.contains("fileid") ||!js.contains("filename") ||!js.contains("sender") ||!js.contains("receiver") ||!js.contains("blocks")){
                cout<<"resume info missing"<<endl;
                return;
            }

            int fileid=js["fileid"];
            string filename=js["filename"];
            string sender =js["sender"];
            string receiver=js["receiver"];
            long long filesize = js["filesize"];

            vector<int> blocks;
                for(auto& b:js["blocks"]){
                    blocks.push_back(b);
                    //cout<<"already received block:"<<b<<endl;
                }
         FileManager::instance().resumeReceive(fileid,sender,filename,filesize,blocks);
         
         cout << "resume receive initialized"<< endl;
         cout << "already received:"<< blocks.size()<< " blocks"<< endl;
         //FileClient::instance().sendFile(fd,fileid,filename,receiver,"user","", blocks);
         cout << "等待发送方继续发送..."<< endl;
         cout<<"============================"<<endl;
}  
   
        //文件分片 ACK
        else if(msgid == FILE_BLOCK_ACK){
            if(!js.contains("fileid") ||!js.contains("blockid")){
                cout << "FILE_BLOCK_ACK lack params" << endl;
                return;
            }
            int fileid = js["fileid"];
            int blockid = js["blockid"];

            FileClient::instance().notifyBlockAck(fileid,blockid);
            
            cout << "========== FILE BLOCK ACK ==========" << endl;
            cout << "fileid   = " << fileid << endl;
            cout << "blockid  = " << blockid << endl;
            cout << "====================================" << endl;
            return;
}
        //普通响应
        else{
            cout<<"\n\n==========收到消息=========="<<endl;
            cout<<js<<endl;
            cout<<"============================"<<endl;
        }  

    }