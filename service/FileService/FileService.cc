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
#include "../../netlib/base/Logger/Logger.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include "../../model/FriendModel/FriendModel.h"
#include "../../model/FriendBlockModel/FriendBlockModel.h"


using namespace std;
json FileService::sendFileRequest(const json& js,TcpConnection* conn){
    json res;
    res["msgid"] = SEND_FILE_REQUEST_ACK;
    // 1. 检查基本参数
    if(!js.contains("targetType") ||!js.contains("filename") ||!js.contains("filesize")){
        LOG_ERROR<<"发送文件请求缺少参数";
        res["errno"] = 1,res["message"] ="send file request lack params";
        return res;
    }

    string fromname=conn->getUsername();
    string filename=js["filename"];
    ll filesize=js["filesize"];
    string targetType=js["targetType"];

    LOG_INFO<<"收到文件发送请求 from="<<fromname<<" filename="<<filename;

    if(!js.contains("filepath") ||!js["filepath"].is_string() ||js["filepath"].get<string>().empty()){
      LOG_ERROR<<"文件路径为空 filename="<<filename;
      res["errno"]=1;
      res["message"]="lack filepath";
    return res;
}
    string filepath=js["filepath"];

    FileModel model;
    // 2. 用户文件
    if(targetType == "user"){
        if(!js.contains("toname")){
            LOG_ERROR<<"发送用户文件缺少目标用户";
            res["errno"] = 1,res["message"] = "lack toname";
            return res;
        }
        string toname = js["toname"];

        FriendModel friendModel;

if(!friendModel.isFriend(fromname,toname)){
    LOG_WARN<<"文件发送失败,不是好友关系 from="<<fromname<<" to="<<toname;
    res["errno"]=1;
    res["message"]="not friend";
    return res;
}

FriendBlockModel blockModel;

if(blockModel.isBlocked(toname,fromname)){
    LOG_WARN<<"文件发送失败,对方屏蔽了自己 from="<<fromname<<" to="<<toname;
    res["errno"]=1;
    res["message"]="you are blocked";
    return res;
}
        // 2.1 先检查有没有未完成的旧文件
        int fileid = model.getUnfinishedFileId(fromname,toname,filename);
        if(fileid >=0){
            // 已经存在未完成文件,不再创建新的 file_info

            LOG_INFO<<"发现未完成文件,继续使用旧文件 fileid="<<fileid;

            if(!model.updateFilePath(fileid,filepath)){
               LOG_ERROR<<"更新文件路径失败 fileid="<<fileid;
               res["errno"]=1;
               res["message"]="update filepath failed";
               return res;
            }
        }else{
            // 没有旧文件，第一次发送
            LOG_INFO<<"创建新的文件记录 filename="<<filename;

            if(!model.saveFileInfo(fromname,toname,"","user",filename,filesize,filepath)){
                LOG_ERROR<<"保存文件信息失败 filename="<<filename;
                res["errno"] = 1,res["message"] ="save file failed";
                return res;
            }
            //获取刚刚创建的fileid
            fileid = model.getFileId(fromname,toname,"","user",filename);

            LOG_INFO<<"创建文件成功 fileid="<<fileid;
        }

        //处理fileid
        if(fileid < 0){
            LOG_ERROR<<"获取fileid失败 filename="<<filename;
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

    LOG_INFO<<"接收方在线,发送文件请求通知 username="<<toname;

    // 接收方在线，直接发送
    target->send(notify.dump());

    // 告诉发送方
    res["errno"] = 0;
    res["message"] = "send file request success";
    res["fileid"] = fileid;

}else{
    // 接收方离线

    LOG_WARN<<"接收方离线,保存离线文件请求 username="<<toname;

    json notify;

    notify["msgid"] = FILE_REQUEST_NOTIFY;
    notify["fromname"] = fromname;
    notify["filename"] = filename;
    notify["filesize"] = filesize;
    notify["targetType"] = "user";
    notify["toname"] = toname;
    notify["fileid"] = fileid;

    if(RedisManager::instance().connect() &&RedisManager::instance().saveOfflineFileRequest(toname, notify)){
        LOG_INFO<<"保存离线文件请求成功 username="<<toname;

        res["errno"] = 0;
        res["message"] = "acceptor offline!";
        res["fileid"] = fileid;
    }else{
        LOG_ERROR<<"保存离线文件请求失败 username="<<toname;

        res["errno"] = 1;
        res["message"] = "save offline file request failed";
    }

    return res;
}
    }
    // 3. 群文件
    else if(targetType == "group"){
        if(!js.contains("groupname")){
            LOG_ERROR<<"群文件请求缺少群名";

            res["errno"] = 1;
            res["message"] = "lack groupname";
            return res;
        }
        string groupname =js["groupname"];

        LOG_INFO<<"发送群文件请求 groupname="<<groupname;

        GroupModel groupModel;
        if(!groupModel.isMember(groupname,fromname)){
            LOG_WARN<<"用户不是群成员,不能发送群文件 username="<<fromname;

            res["errno"] = 1;
            res["message"] = "not group member";
            return res;
        }
        if(!model.saveFileInfo(fromname,"",groupname,"group",filename,filesize,filepath)){
            LOG_ERROR<<"保存群文件信息失败 filename="<<filename;

            res["errno"] = 1;
            res["message"] = "save group file failed";
            return res;
        }

        int fileid =model.getFileId(fromname,"",groupname,"group",filename);

        LOG_INFO<<"创建群文件记录成功 fileid="<<fileid;

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

                LOG_INFO<<"发送群文件通知 member="<<member;

                target->send(notify.dump());
            }else{

                LOG_WARN<<"群文件接收成员离线 member="<<member;

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
        LOG_ERROR<<"未知文件目标类型 targetType="<<targetType;

        res["errno"] = 1;
        res["message"] ="unknown target type";
        return res;
    }
    return res;
}

json FileService::acceptFile(const json& js,TcpConnection* conn){
    json res;
    res["msgid"] = FILE_ACCEPT_ACK;

    // 1. 检查参数
    if(!js.contains("fromname") ||!js.contains("filename")){
        LOG_ERROR << "文件接受请求参数缺失,json=" << js.dump();

        res["errno"] = 1;
        res["message"] = "lack params";
        return res;
    }

    string sender = js["fromname"];
    string acceptor = conn->getUsername();
    string filename = js["filename"];

    LOG_INFO << "收到文件接受请求,发送者=" << sender << ",接收者=" << acceptor << ",文件=" << filename;

    FileModel model;

    string targetType = "user";

    if(js.contains("targetType")){
        targetType = js["targetType"];
    }

    bool exist = false;
    string groupname = "";

    // 2. 检查文件请求是否存在

    if(targetType == "user"){
        exist = model.checkFileRequest(sender,acceptor,filename);
    }else if(targetType == "group"){
        if(!js.contains("groupname")){
            LOG_ERROR << "群文件接受请求缺少群名";

            res["errno"] = 1;
            res["message"] = "lack groupname";
            return res;
        }

        groupname = js["groupname"];
        exist = model.checkGroupFileRequest(sender,groupname,filename);
    }else{
        LOG_ERROR << "未知文件目标类型=" << targetType;

        res["errno"] = 1;
        res["message"] = "unknown target type";
        return res;
    }

    if(!exist){
        LOG_ERROR << "文件请求不存在,发送者=" << sender << ",接收者=" << acceptor << ",文件=" << filename;

        res["errno"] = 1;
        res["message"] = "file request not found";
        return res;
    }
    int fileid = -1;

    if(targetType == "group"){
        fileid = model.getFileId(sender,"",groupname,"group",filename);
    }else{
        fileid = model.getFileId(sender,acceptor,"","user",filename);
    }
    if(fileid < 0){
        LOG_ERROR << "查询文件信息失败,文件=" << filename;

        res["errno"] = 1;
        res["message"] = "file info not found";
        return res;
    }

    LOG_INFO << "文件信息查询成功,fileid=" << fileid << ",文件=" << filename;
    // 发送方不在线时不能先把文件标记为传输中，否则会留下无法继续的脏状态。
    TcpConnection* target =OnlineUserManager::instance().getConnection(sender);

    if(!target){
        LOG_WARN << "文件发送方离线,发送者=" << sender << ",文件=" << filename;

        res["errno"] = 1;
        res["message"] = "sender offline";
        return res;
    }

    // 3. 修改文件状态
    bool update = false;

    if(targetType == "user"){
        update = model.updateFileStatus(sender,acceptor,filename, 1);
    }else if(targetType == "group"){
        update = model.updateFileStatus(sender,groupname,filename,1);
    }

    if(!update){
        LOG_ERROR << "更新文件状态失败,fileid=" << fileid;

        res["errno"] = 1;
        res["message"] = "update file status failed";
        return res;
    }

    LOG_INFO << "文件状态更新成功,fileid=" << fileid;


    // 普通文件也必须建立接收进度记录，后续数据包才能更新断点。
    if(targetType == "user" && !model.saveFileReceiver(fileid, acceptor)){
        LOG_ERROR << "保存文件接收记录失败,fileid=" << fileid << ",接收者=" << acceptor;

        res["errno"] = 1;
        res["message"] = "save file receiver failed";
        return res;
    }
    // 6. 组装 FILE_ACCEPT_NOTIFY

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

    if(targetType == "group"){
        notify["groupname"] = groupname;
    }
    // 7. 获取已经接收的大小

    string receiver;

    if(targetType == "user"){
        // 用户文件：
        // 接收者就是当前登录并接受文件的人
        receiver = acceptor;
    }else{
        // 群文件：
        receiver = acceptor;
    }

    long long size =model.getReceivedSize(fileid, receiver);
    notify["received_size"] = size;

    LOG_INFO << "文件接受通知准备发送,fileid=" << fileid<< ",发送者=" << sender<< ",接收者=" << receiver<< ",文件=" << filename<< ",已接收大小=" << size;

    // 8. 通知发送者开始发送
    target->send(notify.dump());

    LOG_INFO << "文件接受成功,通知发送方开始传输,fileid=" << fileid;

    res["errno"] = 0;
    res["message"] = "accept file request success";
    res["fileid"] = fileid;

    return res;
}

void FileService::receiveFileData(const FilePacket& packet,TcpConnection* conn){
    json js=packet.info;

    if(!js.contains("fileid")||!js.contains("filesize") ||!js.contains("offset") || !js.contains("targetType")){
        LOG_ERROR << "接收文件数据参数缺失";
        return;
    }

    int fileid=js["fileid"];
    string targetType=js["targetType"];

    if(targetType!="user" && targetType!="group"){
        LOG_ERROR << "未知文件目标类型=" << targetType;
        return;
    }

    long long filesize=js["filesize"];

    string receiver;
    if(targetType=="user"){
        if(!js.contains("toname")){
            LOG_ERROR << "用户文件缺少接收者";
            return;
        }
        receiver=js["toname"];
    }else{
        if(!js.contains("receiver")){
            LOG_ERROR << "群文件缺少接收者";
            return;
        }
        receiver=js["receiver"];
    }

    TcpConnection* target =OnlineUserManager::instance().getConnection(receiver);
    if(!target){
        LOG_WARN << "文件接收方离线,接收者=" << receiver << ",fileid=" << fileid;
        return;
    }

    string data =MessageCodec::encodeBinary(packet.msgid,packet.info,packet.data);

    if(target->sendBinary(data)){
        FileModel model;

        long long offset =js["offset"];
        long long size =offset + packet.data.size();
        long long old =model.getReceivedSize(fileid,receiver);
        long long nextSize = size > old ? size : old;

        if(nextSize>old){
            model.updateReceivedSize(fileid,receiver,nextSize);

            LOG_INFO << "更新文件接收进度,fileid=" << fileid<< ",接收者=" << receiver<< ",当前大小=" << nextSize;
        }
    }else{
        LOG_ERROR << "发送文件数据失败,fileid=" << fileid<< ",接收者=" << receiver;
    }
}
void FileService::finishFile(const json& js,TcpConnection* conn){

    if(!js.contains("filename") ||!js.contains("fromname") ||!js.contains("filesize") ||!js.contains("fileid")){
        LOG_ERROR << "文件完成通知参数缺失";
        return;
    }

    string toname="";
    if(js.contains("toname"))toname=js["toname"];

    string targetType="user";
    if(js.contains("targetType"))targetType=js["targetType"];

    if(targetType!="user" && targetType!="group"){
        LOG_ERROR << "文件完成通知目标类型错误,targetType=" << targetType;
        return;
    }

    string receiver;

    if(targetType=="user")receiver=toname;
    else if(targetType=="group"){
        if(!js.contains("receiver")){
            LOG_ERROR << "群文件完成通知缺少接收者";
            return;
        }
        receiver=js["receiver"];
    }

    string filename = js["filename"];
    string fromname = js["fromname"];
    long long filesize = js["filesize"];
    int fileid = js["fileid"];

    FileModel model;

    long long receivedSize =model.getReceivedSize(fileid,receiver);

    LOG_INFO << "检查文件接收完成状态,fileid=" << fileid<< ",已接收大小=" << receivedSize<< ",文件大小=" << filesize;

    // 文件没有接收完成
    if(receivedSize < filesize){
        LOG_WARN << "文件未接收完成,fileid=" << fileid<< ",当前大小=" << receivedSize<< ",需要大小=" << filesize;


        if(targetType=="user"){
            model.updateFileStatus(fromname,toname,filename,1);
        }else if(targetType=="group"){
            model.updateFileReceiver(fileid,receiver,1);
        }
        return;
    }

    if(targetType=="user")
        model.updateFileStatus(fromname,toname,filename,2);
    else if(targetType=="group"){
        model.updateFileReceiver(fileid,receiver,2);

        //检查所有接收者
        if(model.checkAllReceiverFinish(fileid)){
            model.updateFileStatus(fromname,js["groupname"],filename,2);
            LOG_INFO << "群文件所有接收者完成,fileid=" << fileid;
        }
    }
    LOG_INFO << "文件传输完成,发送者=" << conn->getUsername()<< ",文件=" << filename<< ",fileid=" << fileid;


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
        LOG_ERROR << "查询断点续传参数缺失";

        res["errno"]=1;
        res["message"]="lack params";
        return res;
    }

    string filename=js["filename"];
    string receiver=conn->getUsername();

    if(js["receiver"] != receiver){
        LOG_ERROR << "断点续传接收者校验失败,请求接收者=" << js["receiver"]<< ",当前用户=" << receiver;

        res["errno"]=1;
        res["message"]="receiver mismatch";
        return res;
    }

    string fromname;
    int fileid=-1;
    long long filesize=0;

    FileModel model;

    if(!model.getUnfinishedFileInfo(receiver,filename,fromname,fileid,filesize)){
        LOG_WARN << "未找到未完成文件,接收者=" << receiver<< ",文件=" << filename;

        res["errno"]=1;
        res["message"]="file not found";
        return res;
    }

    long long receivedSize=model.getReceivedSize(fileid,receiver);

    LOG_INFO << "查询断点续传信息,fileid="<< fileid<< ",已接收大小="<< receivedSize<< ",文件大小="<< filesize;


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

        LOG_INFO << "发送断点续传通知,fileid="<< fileid<< ",发送者="<< fromname;
    }else{
        LOG_WARN << "发送方离线,无法发送续传通知,发送者="<< fromname;
    }

    return res;
}
void FileService::updateFileOffset(const json& js,TcpConnection* conn){
    if(!js.contains("fileid")||!js.contains("offset")){
        LOG_ERROR << "更新断点参数缺失";
        return;
    }
    int fileid=js["fileid"];
    long long offset=js["offset"];
}
