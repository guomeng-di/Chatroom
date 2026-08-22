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
    
    string sender=js["fromname"];//源文件请求发送者
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
    update=model.updateFileStatus(sender,groupname,filename,1);
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

        string receiver;

if(js["targetType"]=="group") receiver=js["groupname"];
else receiver=js["receiver"];

        long long size=model.getReceivedSize(fileid,receiver);
        notify["received_size"]=size;
        target->send(notify.dump());
        res["errno"]=0;
        res["message"]="accept file request success";

    return res;
}
void FileService::receiveFileData(const FilePacket& packet,TcpConnection* conn){
json js=packet.info;

int fileid=js["fileid"];
string targetType=js["targetType"];
long long filesize=js["filesize"];

string receiver;
if(targetType=="user")receiver=js["toname"];
else receiver=js["receiver"];

TcpConnection* target =OnlineUserManager::instance().getConnection(receiver);
if(!target) return;

string data =MessageCodec::encodeBinary(packet.msgid,packet.info,packet.data);

if(target->sendBinary(data)){
    FileModel model;

    long long offset =js["offset"];
    long long size =offset + packet.data.size();
    long long old =model.getReceivedSize(fileid,receiver);

    if(size>old)
        model.updateReceivedSize(fileid,receiver,size);
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

    string receiver;
    if(targetType=="user")receiver=toname;
    else if(targetType=="group")receiver=js["receiver"];

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
    string sender=conn->getUsername();
    string receiver=js["receiver"];

    FileModel model;
    int fileid=model.getUnfinishedFileId(sender,receiver,filename);
    if(fileid<0){
        res["errno"]=1;
        res["message"]="file not found";
        return res;
    }
    long long receivedSize=model.getReceivedSize(fileid,receiver);
    long long filesize=model.getFileSize(fileid);

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

    return res;
}
void FileService::updateFileOffset(const json& js,TcpConnection* conn){
    if(!js.contains("fileid")||!js.contains("offset")){
        return;
    }

    int fileid=js["fileid"];
    long long offset=js["offset"];
}
