#include "FileService.h"
#include "../../model/FileModel/FileModel.h"
#include "../../protocol/MsgId.h"
#include <filesystem>
#include <iostream>
#include "../../src/config.h"
#include "../../model/GroupModel/GroupModel.h"
#include <arpa/inet.h>
#include "../../client/FileClient/FileClient.h"
#include "../../protocol/MessageCodec/MessageCodec.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h"
#include "../../manager/RedisManager/RedisManager.h"
#include "../../netlib/base/Logger.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include "../../model/FriendModel/FriendModel.cc"
#include "../../model/FriendBlockModel/FriendBlockModel.cc"


using namespace std;
json FileService::sendFileRequest(const json& js,TcpConnection* conn){
    cout << "sendFileRequest receive:" << endl;
    cout << js.dump(4) << endl;
    json res;
    res["msgid"] = SEND_FILE_REQUEST_ACK;
    // 1. 检查基本参数
    if(!js.contains("fromname") ||!js.contains("targetType") ||!js.contains("filename") ||!js.contains("filesize")){
        Logger::instance().error("send file request lack params");
        res["errno"] = 1,res["message"] ="send file request lack params";
        return res;
    }
    //string fromname = js["fromname"];
    string fromname = conn->getUsername();
    string filename = js["filename"];
    ll filesize = js["filesize"];
    string targetType = js["targetType"];

    FileModel model;
    // 2. 用户文件
    if(targetType == "user"){
        if(!js.contains("toname")){
            res["errno"] = 1,res["message"] = "lack toname";
            return res;
        }
        string toname = js["toname"];
        int fileid = -1;

        FriendModel friendModel;

if(!friendModel.isFriend(fromname,toname))
{
    res["errno"]=1;
    res["message"]="not friend";
    return res;
}

FriendBlockModel blockModel;

if(blockModel.isBlocked(toname,fromname))
{
    res["errno"]=1;
    res["message"]="you are blocked";
    return res;
}


        // 2.1 先检查有没有未完成的旧文件
        fileid = model.getUnfinishedFileId(fromname,toname,filename);
        if(fileid >=0){
            // 已经存在未完成文件,不再创建新的 file_info
            cout << "\n==========发现未完成文件==========" << endl;
            cout << "fileid   = " << fileid << endl;
            cout << "fromname = " << fromname << endl;
            cout << "toname   = " << toname << endl;
            cout << "filename = " << filename << endl;
            cout << "继续使用旧 fileid" << endl;
            cout << "==================================" << endl;
        }else{
            // 没有旧文件，第一次发送
            if(!model.saveFileInfo(fromname,toname,"","user",filename,filesize)){
                Logger::instance().error("save file failed");
                res["errno"] = 1,res["message"] ="save file failed";
                return res;
            }
            //获取刚刚创建的fileid
            fileid = model.getFileId(fromname,toname,"","user",filename);
            cout << "new fileid="<< fileid<< endl;
        }

        //处理fileid
        if(fileid < 0){
            Logger::instance().error("get fileid failed");
            res["errno"] = 1,res["message"] ="get fileid failed";
            return res;
        }
        // 2.2 通知接收方
        TcpConnection* target =OnlineUserManager::instance().getConnection(toname);
        if(target){
            json notify;
            notify["msgid"]=FILE_REQUEST_NOTIFY;
            notify["fromname"]=fromname;
            notify["filename"]=filename;
            notify["filesize"]=filesize;
            notify["targetType"]="user";
            notify["toname"]=toname;
            notify["fileid"]=fileid;

            target->send(notify.dump());
            //告诉发送方
            res["errno"] = 0;
            res["message"] ="send file request success";
            res["fileid"] =fileid;
        }else{
            // 接收方离线
            Logger::instance().info("file receiver offline:"+ toname);
            if(RedisManager::instance().connect()){
                if(RedisManager::instance().saveOfflineFileRequest(toname,js)){
                    res["errno"] = 0;
                    res["message"] ="acceptor offline!";
                }
            }
            return res;
        }
    }
    // 3. 群文件
    else if(targetType == "group"){
        if(!js.contains("groupname")){
            res["errno"] = 1;
            res["message"] = "lack groupname";
            return res;
        }
        string groupname =js["groupname"];

        GroupModel groupModel;
        if(!groupModel.isMember(groupname,fromname)){
            res["errno"] = 1;
            res["message"] = "not group member";
            return res;
        }
        if(!model.saveFileInfo(fromname,"",groupname,"group",filename,filesize)){
            Logger::instance().error("save group file failed");
            res["errno"] = 1;
            res["message"] = "save group file failed";
            return res;
        }
        int fileid =model.getFileId(fromname,"",groupname,"group",filename);
        auto members =groupModel.getMembers(groupname);
        // 给每个成员发送文件请求
        for(auto& member : members){
            if(member == fromname)continue;
            model.saveFileReceiver(fileid,member);

            json notify;
            notify["msgid"] =FILE_REQUEST_NOTIFY;
            notify["fromname"] =fromname;
            notify["filename"] =filename;
            notify["filesize"] =filesize;
            notify["groupname"] =groupname;
            notify["targetType"] ="group";
            notify["fileid"] =fileid;

            TcpConnection* target =OnlineUserManager::instance().getConnection(member);
            if(target){
                cout<< "send group file request to "<< member<< endl;
                target->send(notify.dump());
            }else{
                cout<< "文件! group file receiver offline: "<< member<< endl;
                if(RedisManager::instance().connect()){
                    RedisManager::instance().saveOfflineFileRequest(member,notify);
                }
            }
        }
        res["errno"] = 0;
        res["message"] ="group file request success";
        res["fileid"] =fileid;
        return res;
    }
    // 4. 未知 targetType
    else{
        res["errno"] = 1;
        res["message"] ="unknown target type";
        return res;
    }
    return res;
}
json FileService::acceptFile(const json& js,TcpConnection* conn){
    json res;
    res["msgid"]=FILE_ACCEPT_ACK;
    if(!js.contains("fromname")||!js.contains("filename")){
        Logger::instance().error("accept file request lack params");
        res["errno"]=1;
        res["message"]="lack params";
        return res;
    }
    
    string sender=conn->getUsername();
    //string sender=js["fromname"];//源文件请求发送者
    string acceptor=conn->getUsername();//我同意你的请求
    string filename=js["filename"];

    Logger::instance().info("accept sender="+sender);
    Logger::instance().info("accept acceptor="+acceptor);

    FileModel model;
    
    string targetType="user";
    if(js.contains("targetType"))  targetType=js["targetType"];
    bool exist=false;
    string groupname="";

    if(targetType=="user") exist=model.checkFileRequest(sender,acceptor,filename);
    else if(targetType=="group"){
        groupname=js["groupname"];
        exist=model.checkGroupFileRequest(sender,groupname,filename);
    }
    if(!exist){
        Logger::instance().error("file request not found");
        res["errno"]=1;
        res["message"]="file request not found";
        return res;
    }
    //先修改状态
bool update=false;
if(targetType=="user") update=model.updateFileStatus(sender,acceptor,filename,1);
else if(targetType=="group"){
    model.getFileId(sender,"",groupname,"group",filename);
    update=model.updateGroupFileStatus(sender,groupname,filename,1);
}
if(!update){
    res["errno"]=1;
    res["message"]="update file status failed";
    return res;
}

//再找发送者
TcpConnection* target=OnlineUserManager::instance().getConnection(sender);
if(!target){
    res["errno"]=1;
    res["message"]="sender offline";
    return res;
}

        json notify;
        notify["msgid"]=FILE_ACCEPT_NOTIFY;
        notify["sender"]=sender;
        notify["receiver"]=acceptor;
        notify["message"] = "accept file request";
        notify["filename"]=filename;

        int fileid=-1;
        if(targetType=="group")  fileid=model.getFileId(sender,"",groupname, "group",filename );
        else fileid=model.getFileId(sender,acceptor,"","user",filename);

        notify["fileid"]=fileid;
        if(fileid<0){
            res["errno"]=1;
            res["message"]="file info not found";
            return res;
        }

        notify["targetType"]=targetType;
        notify["receiver"]=acceptor;
        if(targetType=="group") notify["groupname"]=groupname;
        vector<int> blocks=model.getReceivedBlocks(fileid,acceptor);
        notify["blocks"]=blocks;
        target->send(notify.dump());
        res["errno"]=0;
        res["message"]="accept file request success";

    return res;
}
void FileService::receiveFileData(const FilePacket& packet,TcpConnection* conn){
    Logger::instance().info("packet msgid=" + to_string(packet.msgid));
    json js = packet.info;
    //int fileid = js["fileid"];
    string filename = js["filename"];
    int blockid = js["blockid"];
    string targetType = js["targetType"];
    string toname;
    string groupname;
    if(targetType == "user")toname = js["toname"];
    else if(targetType == "group")groupname = js["groupname"];

    string sendData =MessageCodec::encodeBinary(packet.msgid,packet.info,packet.data);
    // 用户文件
    if(targetType == "user"){
        TcpConnection* target =OnlineUserManager::instance().getConnection(toname);
        if(!target){
            Logger::instance().error("receiver offline:"+ toname+ " block="+ to_string(blockid));
            return;
        }
        // 转发给接收方
        bool ok = target->sendBinary(sendData);
        if(!ok){
            Logger::instance().error("forward file block failed block="+ to_string(blockid));
            return;
        }
        cout << "forward block:" << blockid<< " to "<< toname<< endl;
    }
    // 群文件
    else if(targetType == "group"){
        string receiver = js["receiver"];
        TcpConnection* target =OnlineUserManager::instance().getConnection(receiver);
        if(!target){
            Logger::instance().error("group receiver offline:"+ receiver+ " block="+ to_string(blockid));
            return;
        }
        bool ok =target->sendBinary(sendData);
        if(!ok){
            Logger::instance().error("send group file block failed");
            return;
        }
        cout << "forward group block:"<< blockid<< " to "<< receiver << endl;
    }
}
void FileService::sendFileData( const json& js,TcpConnection* conn){
    Logger::instance().info("send file data");
}
void FileService::finishFile(const json& js,TcpConnection* conn){
    if(!js.contains("filename") ||!js.contains("fromname") ||!js.contains("filesize") ||!js.contains("fileid")){
        Logger::instance().error("finish file lack params");
        return;
    }

    string toname="";
    if(js.contains("toname"))toname=js["toname"];
    string targetType="user";
    if(js.contains("targetType"))targetType=js["targetType"];

    string receiver;
    if(targetType=="user")receiver=toname;
    else if(targetType=="group")receiver=js["receiver"];

    string filename = js["filename"];
    string fromname = js["fromname"];
    //string toname = js["toname"];
    long long filesize = js["filesize"];
    int fileid = js["fileid"];
    const long long BLOCK_SIZE = 4096;
    long long expectedBlocks =(filesize + BLOCK_SIZE - 1)/ BLOCK_SIZE;
    FileModel model;
    vector<int> blocks =model.getReceivedBlocks(fileid,receiver);

    cout << "finish check:"<< " received="<< blocks.size()<< " expected="<< expectedBlocks<< endl;

    // block没有收完整
    if(static_cast<long long>(blocks.size())!= expectedBlocks){
        if(targetType=="user") model.updateFileStatus(fromname,toname,filename,1);
        else model.updateFileReceiver(fileid,receiver,1);
        return;
    }

    if(targetType=="user")model.updateFileStatus(fromname,toname,filename,2);
    else if(targetType=="group"){
        model.updateFileReceiver(fileid,receiver,2);
        //检查所有接收者
        if(model.checkAllReceiverFinish(fileid)){
            model.updateGroupFileStatus(fromname,js["groupname"],filename,2);
            cout<<"all group receiver finish"<<endl;
        }
    }


    Logger::instance().info(conn->getUsername()+ " file finish:"+ filename);

    json notify;
    notify["msgid"] =FILE_FINISH_NOTIFY;
    notify["errno"] = 0;
    notify["filename"] = filename;
    notify["fromname"] = fromname;
    notify["toname"] = toname;
    notify["message"] ="file send success";

    conn->send(notify.dump());
}
json FileService::querySendFileBlock(const json& js,TcpConnection* conn){
    json res;
    res["msgid"] = FILE_RESUME_ACCEPT;

    if(!js.contains("filename") ||!js.contains("sender") ||!js.contains("receiver")){
        res["errno"]=1;
        res["message"]="lack params";
        return res;
    }
    string filename=js["filename"];
    string sender=conn->getUsername();
    //string sender=js["sender"];
    string receiver=js["receiver"];
    cout << "\n========== query file block ==========" << endl;
    cout << "sender=" << sender << endl;
    cout << "receiver=" << receiver << endl;
    cout << "filename=" << filename << endl;

    FileModel model;
    int fileid = model.getUnfinishedFileId(sender,receiver,filename);
    cout << "unfinished fileid=" << fileid << endl;
    if(fileid < 0){
        res["errno"] = 1;
        res["message"] = "unfinished file not found";
        return res;
    }
    long long filesize = model.getFileSize(fileid);
    if(filesize < 0){
        res["errno"] = 1;
        res["message"] = "get filesize failed";
        return res;
    }
    vector<int> blocks =model.getReceivedBlocks(fileid, receiver);

    cout << "\n===== FILE RESUME =====" << endl;
    cout << "fileid   = " << fileid << endl;
    cout << "sender   = " << sender << endl;
    cout << "receiver = " << receiver << endl;
    cout << "filename = " << filename << endl;
    cout << "filesize = " << filesize << endl;
    cout << "block数  = "<< blocks.size()<< endl;

    cout << "blocks  = ";
    for(int b : blocks)
        cout << b << " ";

    cout << endl;
    // 给发送方
    TcpConnection* senderConn =OnlineUserManager::instance().getConnection(sender);

    if(senderConn){
        json notify;

        notify["msgid"] = FILE_RESUME_SEND;
        notify["fileid"] = fileid;
        notify["filename"] = filename;
        notify["filesize"] = filesize;   
        notify["receiver"] = receiver;
        notify["blocks"] = blocks;

        senderConn->send(notify.dump());
        cout << "resume notify sent to sender" << endl;
    }
    // 给接收方
    res["errno"] = 0;
    res["message"] = "query success";
    res["fileid"] = fileid;
    res["filename"] = filename;
    res["filesize"] = filesize;   // ★ 关键
    res["sender"] = sender;
    res["receiver"] = receiver;
    res["blocks"] = blocks;
    cout << "===================================="<< endl;
    return res;
}

void FileService::fileBlockAck(const json& js,TcpConnection* conn){
    if(!js.contains("fileid")||!js.contains("filename")||!js.contains("blockid")||!js.contains("receiver")||!js.contains("fromname")){
        Logger::instance().error("file block ack lack params");
        return;
    }
    int fileid =js["fileid"];
    string filename =js["filename"];
    int blockid =js["blockid"];
    string receiver =js["receiver"];
    string sender=conn->getUsername();
    //string sender = js["fromname"];
    cout << "========== FILE BLOCK ACK =========="<< endl;
    cout << "fileid   = "<< fileid<< endl;
    cout << "filename = "<< filename<< endl;
    cout << "sender   = "<< sender<< endl;
    cout << "receiver = "<< receiver<< endl;
    cout << "blockid  = "<< blockid<< endl;
    // FileModel model;
    // if(!model.saveFileBlock(fileid,filename,receiver,blockid)){
    //     Logger::instance().error("save file block failed:"+ to_string(blockid));
    //     return;
    // }
    // cout<<"save block success:"<< blockid<< endl;
    //转发ACK给发送方
    TcpConnection* senderConn=OnlineUserManager::instance().getConnection(sender);
    if(!senderConn){
        Logger::instance().error("sender offline:"+sender);
        return ;
    }
    //string ackData=MessageCodec::encode(js.dump());
    senderConn->send(js.dump());
    cout<<"forward ACK to sender:"<<sender<<" block="<<blockid<<endl;
    FileModel model;
    if(!model.saveFileBlock(fileid,filename,receiver,blockid)){
        Logger::instance().error("save file block failed:"+ to_string(blockid));
        return;
    }
    cout<<"save block success:"<< blockid<< endl;
    cout<<"===================================="<< endl;
}