#include "FileService.h"
#include "../../model/FileModel/FileModel.h"
#include "../../protocol/MsgId.h"
#include <filesystem>
#include <iostream>
#include "../../src/config.h"
#include <arpa/inet.h>
#include "../../protocol/MessageCodec/MessageCodec.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h"
#include "../../manager/RedisManager/RedisManager.h"
#include "../../netlib/base/Logger.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"


using namespace std;
json FileService::sendFileRequest(const json& js,TcpConnection* conn){
    json res;
    res["msgid"]=SEND_FILE_REQUEST_ACK;
    if(!js.contains("fromname")||!js.contains("toname")||!js.contains("filename")||!js.contains("filesize")){
        Logger::instance().error("send file request lack params");
        res["errno"]=1;
        res["message"]="send file request lack params";
        return res;
    }
    string fromname=js["fromname"];
    string toname= js["toname"];
    string filename = js["filename"];
    ll filesize = js["filesize"];
    FileModel model;
    if(!model.saveFileInfo(fromname,toname,filename,filesize)){
        Logger::instance().error("save file failed");
        res["errno"]=1;
        res["message"]="save file failed";
        return res;
    }
    TcpConnection* target=OnlineUserManager::instance().getConnection(toname);
    if(target){
        json notify;
        notify["msgid"]=FILE_REQUEST_NOTIFY;
        notify["fromname"]=fromname;
        notify["filename"]=filename;
        notify["filesize"]=filesize;

        target->send(notify.dump());

        res["errno"]=0;
        res["message"]="send file request success";
    }else{
        Logger::instance().info("file receiver offline:"+toname);
        if(RedisManager::instance().connect()){
          if(RedisManager::instance().saveOfflineFileRequest(toname,js)){
        res["errno"]=0;
        res["message"]="acceptor offline!";
        return res;
     }
    }
        res["errno"]=1;
        res["message"]="send file request failed";
    }
    return res;
}
json FileService::acceptFile(const json& js,TcpConnection* conn){
    json res;
    res["msgid"]=FILE_ACCEPT_ACK;
    if(!js.contains("fromname")){
        Logger::instance().error("accept file request lack params");
        res["errno"]=1;
        res["message"]="lack params";
        return res;
    }
    
    string sender=js["fromname"];//源文件请求发送者
    string acceptor=conn->getUsername();//我同意你的请求
    Logger::instance().info("accept sender="+sender);
    Logger::instance().info("accept acceptor="+acceptor);

    FileModel model;
    string filename=model.getFileName(sender,acceptor);//获取文件名
    TcpConnection* target=OnlineUserManager::instance().getConnection(sender);
    if(target){
        Logger::instance().info("target found");
        if(!model.updateFileStatus(sender,acceptor,1)){
            Logger::instance().error("update file status failed");
            res["errno"]=1;
            res["message"]="update file status failed";
            return res;
        }


        json notify;
        notify["msgid"]=FILE_ACCEPT_NOTIFY;
        notify["fromname"]=acceptor;
        notify["message"] = "accept file request";
        notify["filename"]=filename;

        target->send(notify.dump());

        res["errno"]=0;
        res["message"]="accept file request success";
    }else{
        res["errno"]=1;
        res["message"]="sender offline";
    }
    return res;
}
void FileService::receiveFileData(const FilePacket& packet,TcpConnection* conn){
    
    Logger::instance().info("packet msgid="+to_string(packet.msgid));
    Logger::instance().info("filename="+packet.info.dump());

    json js=packet.info;
    string filename = js["filename"];
    string toname = js["toname"];
    //string fromname = js["fromname"];
    int blockid = js["blockid"];
    Logger::instance().info("receive file block:"+ filename+ " block="+ to_string(blockid));
    string fileData = packet.data;

    string dir=FILE_ROOT+toname+"/recv/";
    filesystem::create_directories(dir);
    string path=dir+filename;
    ofstream file(path,ios::binary | ios::app);
    if(!file.is_open()){
        Logger::instance().error("open file failed");
        return;
    }
    file.write(fileData.data(),fileData.size());
    file.close();
    Logger::instance().info("write file block success");
}
void FileService::sendFileData( const json& js,TcpConnection* conn){
    Logger::instance().info("send file data");
}
void FileService::finishFile(const json& js,TcpConnection* conn){
    string filename =js["filename"];
    Logger::instance().info(conn->getUsername()+" file finish:"+filename);

    json notify;
    notify["msgid"]=FILE_FINISH_MSG;
    notify["filename"]=filename;
    notify["message"]="file send success";

    conn->send(notify.dump());
}
