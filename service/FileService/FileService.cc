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
#include "../../model/FriendModel/FriendModel.h"
#include "../../model/FriendBlockModel/FriendBlockModel.h"


using namespace std;
json FileService::sendFileRequest(const json& js,TcpConnection* conn){
    json res;
    res["msgid"] = SEND_FILE_REQUEST_ACK;
    // 1. 检查基本参数
    if(!js.contains("targetType") ||!js.contains("filename") ||!js.contains("filesize")){
        Logger::instance().error("send file request lack params");
        res["errno"] = 1,res["message"] ="send file request lack params";
        return res;
    }

    string fromname=conn->getUsername();
    string filename=js["filename"];
    ll filesize=js["filesize"];
    string targetType=js["targetType"];
    if(!js.contains("filepath") ||
       !js["filepath"].is_string() ||
       js["filepath"].get<string>().empty()){
      res["errno"]=1;
      res["message"]="lack filepath";
    return res;
}
    string filepath=js["filepath"];

    FileModel model;
    // 2. 用户文件
    if(targetType == "user"){
        if(!js.contains("toname")){
            res["errno"] = 1,res["message"] = "lack toname";
            return res;
        }
        string toname = js["toname"];

        FriendModel friendModel;

if(!friendModel.isFriend(fromname,toname))
{
    res["errno"]=1;
    res["message"]="not friend";
    return res;
}

FriendBlockModel blockModel;

if(blockModel.isBlocked(toname,fromname)){
    res["errno"]=1;
    res["message"]="you are blocked";
    return res;
}
        // 2.1 先检查有没有未完成的旧文件
        int fileid = model.getUnfinishedFileId(fromname,toname,filename);
        if(fileid >=0){
            // 已经存在未完成文件,不再创建新的 file_info
            cout << "\n==========发现未完成文件==========" << endl;
            cout << "fileid   = " << fileid << endl;
            cout << "fromname = " << fromname << endl;
            cout << "toname   = " << toname << endl;
            cout << "filename = " << filename << endl;
            cout << "继续使用旧 fileid" << endl;
            cout << "==================================" << endl;
            if(!model.updateFilePath(fileid,filepath)){
               res["errno"]=1;
               res["message"]="update filepath failed";
               return res;

            }
        }else{
            // 没有旧文件，第一次发送
            if(!model.saveFileInfo(fromname,toname,"","user",filename,filesize,filepath)){
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
json notify;
notify["msgid"] = FILE_REQUEST_NOTIFY;
notify["fromname"] = fromname;
notify["filename"] = filename;
notify["filesize"] = filesize;
notify["targetType"] = "user";
notify["toname"] = toname;
notify["fileid"] = fileid;

TcpConnection* target =
    OnlineUserManager::instance().getConnection(toname);

if(target){

    // 接收方在线，直接发送
    target->send(notify.dump());

    // 告诉发送方
    res["errno"] = 0;
    res["message"] = "send file request success";
    res["fileid"] = fileid;

}else{
    // 接收方离线
    cout << "========== FILE RECEIVER OFFLINE ==========" << endl;
    cout << "fromname=" << fromname << endl;
    cout << "toname=" << toname << endl;
    cout << "filename=" << filename << endl;
    cout << "fileid=" << fileid << endl;

    json notify;

    notify["msgid"] = FILE_REQUEST_NOTIFY;
    notify["fromname"] = fromname;
    notify["filename"] = filename;
    notify["filesize"] = filesize;
    notify["targetType"] = "user";
    notify["toname"] = toname;
    notify["fileid"] = fileid;

    cout << "offline file notify="
         << notify.dump()
         << endl;

    if(RedisManager::instance().connect() &&
       RedisManager::instance().saveOfflineFileRequest(toname, notify)){
        res["errno"] = 0;
        res["message"] = "acceptor offline!";
        res["fileid"] = fileid;
    }else{
        res["errno"] = 1;
        res["message"] = "save offline file request failed";
    }
    cout << "============================================"<< endl;

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
        if(!model.saveFileInfo(fromname,"",groupname,"group",filename,filesize,filepath)){
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
json FileService::acceptFile(const json& js,TcpConnection* conn)
{
    json res;
    res["msgid"] = FILE_ACCEPT_ACK;

    // ============================
    // 1. 检查参数
    // ============================
    // fileid is optional for compatibility with older clients. The server
    // resolves the canonical id from the persisted file request below.
    if(!js.contains("fromname") ||
       !js.contains("filename"))
    {
        cout << "========== ACCEPT FILE ERROR ==========" << endl;
        cout << "accept file request lack params" << endl;
        cout << "json=" << js.dump(4) << endl;
        cout << "=======================================" << endl;

        res["errno"] = 1;
        res["message"] = "lack params";
        return res;
    }

    string sender = js["fromname"];
    string acceptor = conn->getUsername();
    string filename = js["filename"];

    cout << "========== ACCEPT FILE ==========" << endl;
    cout << "sender=" << sender << endl;
    cout << "acceptor=" << acceptor << endl;
    cout << "filename=" << filename << endl;
    cout << "=================================" << endl;

    FileModel model;

    string targetType = "user";

    if(js.contains("targetType"))
    {
        targetType = js["targetType"];
    }

    bool exist = false;
    string groupname = "";

    // ============================
    // 2. 检查文件请求是否存在
    // ============================

    if(targetType == "user")
    {
        exist = model.checkFileRequest(
            sender,
            acceptor,
            filename
        );
    }
    else if(targetType == "group")
    {
        if(!js.contains("groupname"))
        {
            cout << "group file lack groupname" << endl;

            res["errno"] = 1;
            res["message"] = "lack groupname";
            return res;
        }

        groupname = js["groupname"];

        exist = model.checkGroupFileRequest(
            sender,
            groupname,
            filename
        );
    }
    else
    {
        cout << "unknown target type="
             << targetType
             << endl;

        res["errno"] = 1;
        res["message"] = "unknown target type";
        return res;
    }

    if(!exist)
    {
        cout << "========== ACCEPT FILE ERROR ==========" << endl;
        cout << "file request not found" << endl;
        cout << "sender=" << sender << endl;
        cout << "acceptor=" << acceptor << endl;
        cout << "filename=" << filename << endl;
        cout << "=======================================" << endl;

        res["errno"] = 1;
        res["message"] = "file request not found";
        return res;
    }

    // Resolve the canonical file id from file_info. Older clients do not
    // include fileid in FILE_ACCEPT_MSG, so it cannot be a required input.
    int fileid = -1;
    if(targetType == "group")
    {
        fileid = model.getFileId(
            sender,
            "",
            groupname,
            "group",
            filename
        );
    }
    else
    {
        fileid = model.getFileId(
            sender,
            acceptor,
            "",
            "user",
            filename
        );
    }

    if(fileid < 0)
    {
        cout << "file info not found" << endl;

        res["errno"] = 1;
        res["message"] = "file info not found";
        return res;
    }

    // 发送方不在线时不能先把文件标记为传输中，否则会留下无法继续的脏状态。
    TcpConnection* target =
        OnlineUserManager::instance().getConnection(sender);
    if(!target)
    {
        res["errno"] = 1;
        res["message"] = "sender offline";
        return res;
    }

    // ============================
    // 3. 修改文件状态
    // ============================

    bool update = false;

    if(targetType == "user")
    {
        update = model.updateFileStatus(
            sender,
            acceptor,
            filename,
            1
        );
    }
    else if(targetType == "group")
    {
        update = model.updateFileStatus(
            sender,
            groupname,
            filename,
            1
        );
    }

    if(!update)
    {
        cout << "update file status failed" << endl;

        res["errno"] = 1;
        res["message"] = "update file status failed";
        return res;
    }

    // ============================
    // 4. 查找发送者
    // ============================

    // ============================
    // 5. 查找 fileid
    // ============================

    // 普通文件也必须建立接收进度记录，后续数据包才能更新断点。
    if(targetType == "user" && !model.saveFileReceiver(fileid, acceptor))
    {
        res["errno"] = 1;
        res["message"] = "save file receiver failed";
        return res;
    }

    // ============================
    // 6. 组装 FILE_ACCEPT_NOTIFY
    // ============================

    json notify;

    notify["msgid"] = FILE_ACCEPT_NOTIFY;

    notify["sender"] = sender;
    notify["receiver"] = acceptor;

    notify["message"] = "accept file request";

    notify["filename"] = filename;

    notify["fileid"] = fileid;

    // 仅发送给发送方，用数据库中的路径恢复客户端重启后的发送状态。
    string senderFilepath = model.getFilePath(fileid);
    if(!senderFilepath.empty()){
        notify["filepath"] = senderFilepath;
    }

    notify["targetType"] = targetType;

    if(targetType == "group")
    {
        notify["groupname"] = groupname;
    }

    // ============================
    // 7. 获取已经接收的大小
    // ============================

    string receiver;

    if(targetType == "user")
    {
        // 用户文件：
        // 接收者就是当前登录并接受文件的人
        receiver = acceptor;
    }
    else
    {
        // 群文件：
        // 这里沿用你当前 FileModel 的设计
        receiver = acceptor;
    }

    long long size =
        model.getReceivedSize(fileid, receiver);

    notify["received_size"] = size;

    cout << "========== FILE ACCEPT NOTIFY ==========" << endl;
    cout << "fileid=" << fileid << endl;
    cout << "sender=" << sender << endl;
    cout << "receiver=" << receiver << endl;
    cout << "filename=" << filename << endl;
    cout << "targetType=" << targetType << endl;
    cout << "received_size=" << size << endl;
    cout << "notify=" << notify.dump(4) << endl;
    cout << "=========================================" << endl;

    // ============================
    // 8. 通知发送者开始发送
    // ============================

    target->send(notify.dump());

    res["errno"] = 0;
    res["message"] = "accept file request success";
    res["fileid"] = fileid;

    return res;
}
void FileService::receiveFileData(const FilePacket& packet,TcpConnection* conn){
json js=packet.info;

if(!js.contains("fileid") || !js.contains("filesize") ||
   !js.contains("offset") || !js.contains("targetType")) return;

int fileid=js["fileid"];
string targetType=js["targetType"];
if(targetType!="user" && targetType!="group") return;
long long filesize=js["filesize"];

string receiver;
if(targetType=="user"){
    if(!js.contains("toname")) return;
    receiver=js["toname"];
}else{
    if(!js.contains("receiver")) return;
    receiver=js["receiver"];
}

TcpConnection* target =OnlineUserManager::instance().getConnection(receiver);
if(!target) return;

    string data =MessageCodec::encodeBinary(packet.msgid,packet.info,packet.data);

    if(target->sendBinary(data)){
        FileModel model;

        long long offset =js["offset"];
        long long size =offset + packet.data.size();
        long long old =model.getReceivedSize(fileid,receiver);
        long long nextSize = size > old ? size : old;

        if(nextSize>old)
            model.updateReceivedSize(fileid,receiver,nextSize);
    }
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

    if(targetType!="user" && targetType!="group"){
        Logger::instance().error("finish file unknown target type");
        return;
    }

    string receiver;
    if(targetType=="user")receiver=toname;
    else if(targetType=="group"){
        if(!js.contains("receiver")) return;
        receiver=js["receiver"];
    }

    string filename = js["filename"];
    string fromname = js["fromname"];
    long long filesize = js["filesize"];
    int fileid = js["fileid"];

    FileModel model;
    long long receivedSize =model.getReceivedSize(fileid,receiver);

     cout
    << "========== finish check =========="
    << endl;

    cout
    << "fileid="
    << fileid
    << endl;

    cout
    << "received size="
    << receivedSize
    << endl;

    cout
    << "filesize="
    << filesize
    << endl;

    cout
    << "=================================="
    << endl;
    // 文件没有接收完成

    if(receivedSize < filesize)
    {

        cout<<"file not finish"<<endl;


        if(targetType=="user")
        {
            model.updateFileStatus(
                fromname,
                toname,
                filename,
                1
            );
        }

        else if(targetType=="group")
        {
            model.updateFileReceiver(
                fileid,
                receiver,
                1
            );
        }


        return;
    }


    if(targetType=="user")model.updateFileStatus(fromname,toname,filename,2);
    else if(targetType=="group"){
        model.updateFileReceiver(fileid,receiver,2);
        //检查所有接收者
        if(model.checkAllReceiverFinish(fileid)){
            model.updateFileStatus(fromname,js["groupname"],filename,2);
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

json FileService::queryResumeFile(const json& js,TcpConnection* conn){
    json res;
    res["msgid"]=FILE_RESUME_REPLY;
    if(!js.contains("filename")||!js.contains("receiver")){
        res["errno"]=1;
        res["message"]="lack params";
        return res;
    }

    string filename=js["filename"];
    string receiver=conn->getUsername();
    if(js["receiver"] != receiver){
        res["errno"]=1;
        res["message"]="receiver mismatch";
        return res;
    }
    string fromname;
    int fileid=-1;
    long long filesize=0;

    FileModel model;
    if(!model.getUnfinishedFileInfo(receiver,filename,fromname,fileid,filesize)){
        res["errno"]=1;
        res["message"]="file not found";
        return res;
    }
    long long receivedSize=model.getReceivedSize(fileid,receiver);

    cout<<"========== resume =========="<<endl;
    cout<<"fileid="<<fileid<<endl;
    cout<<"received_size="<<receivedSize<<endl;
    cout<<"filesize="<<filesize<<endl;
    cout<<"============================="<<endl;
    
    res["received_size"]=receivedSize;
    res["errno"]=0;
    res["fileid"]=fileid;
    res["filename"]=filename;
    res["filesize"]=filesize;
    res["sender"]=fromname;
    res["receiver"]=receiver;

    // 发送方可能在创建请求后重启，将数据库中的本地路径放入只发给发送方的续传通知。
    string senderFilepath = model.getFilePath(fileid);

    TcpConnection* senderConn=OnlineUserManager::instance().getConnection(fromname);
    if(senderConn){
        json notify;
        notify["msgid"]=FILE_RESUME_SEND;
        notify["fileid"]=fileid;
        notify["filename"]=filename;
        notify["sender"]=fromname;
        notify["receiver"]=receiver;
        notify["received_size"]=receivedSize;
        notify["filesize"]=filesize;
        if(!senderFilepath.empty()){
            notify["filepath"] = senderFilepath;
        }
        senderConn->send(notify.dump());
    }

    return res;
}
void FileService::updateFileOffset(const json& js,TcpConnection* conn){
    if(!js.contains("fileid")||!js.contains("offset")){
        return;
    }

    int fileid=js["fileid"];
    long long offset=js["offset"];
}
