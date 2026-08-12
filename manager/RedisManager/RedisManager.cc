#include "RedisManager.h"
#include <hiredis/hiredis.h>
#include <iostream>
#include "../../netlib/base/Logger.h"

using namespace std;
RedisManager& RedisManager::instance()
{
    // static局部变量：程序第一次调用才创建，全局仅此一份
    static RedisManager obj;
    return obj;
}
RedisManager::RedisManager(){
    redisContext_=nullptr;
}
RedisManager::~RedisManager(){
    if(redisContext_)
        redisFree((redisContext*)redisContext_);
}
bool RedisManager::connect(){
    redisContext* context =redisConnect("127.0.0.1",6379);
    if(context==nullptr||context->err){
        cout<<"redis connect fail"<<endl;
        return false;
    }
    redisContext_=context;
    cout<<"redis connect success"<<endl;
    return true;
}
bool RedisManager::saveOfflineMessage(const std::string& username,const std::string& message){
    cout<<"save offline message:"<<endl;
    cout<<"user="<<username<<endl;
    cout<<"msg="<<message<<endl;


    if(redisContext_==NULL) return 0;
    redisContext* context=(redisContext*)redisContext_;
    string key="offline:private:"+username;
    redisReply* reply=(redisReply*)redisCommand(context,"RPUSH %s %b",key.c_str(),message.data(),
    message.size());
    if(reply==NULL) return 0;
    freeReplyObject(reply);
    return 1;
}
vector<string> RedisManager::getOfflineMessage(const string& username){
    vector<string> messages;
    if(redisContext_==NULL) return messages;
    redisContext* context=(redisContext*)redisContext_;
    string key="offline:private:"+username;

    cout<<"get offline key="
        <<key
        <<endl;


    redisReply* reply=(redisReply*)redisCommand(context,"LRANGE %s 0 -1",key.c_str());
    if(reply==NULL) return messages;

     cout<<"redis elements="
        <<reply->elements
        <<endl;
        
    for(size_t i=0;i<reply->elements;i++){
        if(reply->element[i]->str) messages.push_back(reply->element[i]->str);
    }
    freeReplyObject(reply);
    return messages;
}
void RedisManager::clearOfflineMessage(const string& username){
    if(redisContext_==nullptr) return;
    redisContext* context =(redisContext*)redisContext_;
    string key="offline:private:"+username;
    redisReply* reply =(redisReply*)redisCommand(context,"DEL %s",key.c_str());
    if(reply) freeReplyObject(reply);
}





bool RedisManager::saveGroupOfflineMessage(const std::string& username,const std::string& message){
    cout<<"save group_offline message:"<<endl;
    cout<<"user="<<username<<endl;
    cout<<"msg="<<message<<endl;


    if(redisContext_==NULL) return 0;
    redisContext* context=(redisContext*)redisContext_;
    string key="offline:group:"+username;
    redisReply* reply=(redisReply*)redisCommand(context,"RPUSH %s %b",key.c_str(),message.data(),
    message.size());
    if(reply==NULL) return 0;
    freeReplyObject(reply);
    return 1;
}
vector<string> RedisManager::getGroupOfflineMessage(const string& username){
    vector<string> messages;
    if(redisContext_==NULL) return messages;
    redisContext* context=(redisContext*)redisContext_;
    string key="offline:group:"+username;

    cout<<"get group offline key="
        <<key
        <<endl;


    redisReply* reply=(redisReply*)redisCommand(context,"LRANGE %s 0 -1",key.c_str());
    if(reply==NULL) return messages;

     cout<<"redis elements="
        <<reply->elements
        <<endl;
        
    for(size_t i=0;i<reply->elements;i++){
        if(reply->element[i]->str) messages.push_back(reply->element[i]->str);
    }
    freeReplyObject(reply);
    return messages;
}
void RedisManager::clearGroupOfflineMessage(const string& username){
    if(redisContext_==nullptr) return;
    redisContext* context =(redisContext*) redisContext_;
    string key="offline:group:"+username;
    redisReply* reply =(redisReply*)redisCommand(context,"DEL %s",key.c_str());
    if(reply) freeReplyObject(reply);
}


bool RedisManager::setOnline(const string& username){
    if(redisContext_==nullptr) return 0;
    redisContext* context=(redisContext*) redisContext_;
    string key="online:"+username;
    redisReply* reply=(redisReply*)redisCommand(context,"SET %s 1",key.c_str());
    if(reply==nullptr){
        Logger::instance().error("redis set online failed");
        return false;
    }
    if(reply) freeReplyObject(reply);
    return 1;
}
bool RedisManager::setOffline(const string& username){
    if(redisContext_==nullptr) return 0;
    redisContext* context=(redisContext*) redisContext_;
    string key="online:"+username;
    redisReply* reply=(redisReply*)redisCommand(context,"DEL %s",key.c_str());
    if(reply==nullptr){
        Logger::instance().error("redis delete online failed");
        return false;
    }
    if(reply) freeReplyObject(reply);
    return 1;

}
bool RedisManager::isOnline(const string& username){
    if(redisContext_==nullptr) return 0;
    redisContext* context=(redisContext*) redisContext_;
    string key="online:"+username;
    redisReply* reply=(redisReply*)redisCommand(context,"EXISTS %s",key.c_str());
    if(reply==nullptr) return false;
    bool result=false;
    if(reply->integer==1) result=true;
    freeReplyObject(reply);
    return result;
}


bool RedisManager::saveVerifyCode(const string& target,const string& code){
    if(redisContext_==nullptr) return 0;
    redisContext* context=(redisContext*) redisContext_;
    string key="verify:"+target;
    redisReply* reply=(redisReply*)redisCommand(context,"SET %s %s EX 300",key.c_str(),code.c_str());
    if(reply==nullptr) return false;
    freeReplyObject(reply);
    return 1;
}
string RedisManager::getVerifyCode(const string& target){
    if(redisContext_==nullptr) return "";
    redisContext* context=(redisContext*) redisContext_;
    string key="verify:"+target;
    Logger::instance().info( "redis get key="+key);
    cout<<"redis get key="<<key<<endl;
    redisReply* reply=(redisReply*)redisCommand(context,"GET %s",key.c_str());
    if(reply==nullptr){
        Logger::instance().error( "redis reply null");
        cout<<"redis reply null";
        return "";
    }

    if(reply->type==REDIS_REPLY_STRING){
        string code=reply->str;
        Logger::instance().info( "redis get code="+code);
        cout<<"redis get code="<<code<<endl;
        freeReplyObject(reply);
        return code;
    }
    Logger::instance().error("redis reply type not string");
    cout<<"redis reply type not string"<<endl;
    freeReplyObject(reply);
    return "";
}
bool RedisManager::deleteVerifyCode(const string& target){
    if(redisContext_==nullptr) return 0;
    redisContext* context=(redisContext*) redisContext_;
    string key="verify:"+target;
    redisReply* reply=(redisReply*)redisCommand(context,"DEL %s",target.c_str());
    freeReplyObject(reply);
    return 1;
}


bool RedisManager::saveOfflineFileRequest(string& username,const json& js){
    if(redisContext_==nullptr) return 0;
    redisContext* context=(redisContext*) redisContext_;
    string key="offline:file:"+username;
    redisReply* reply=(redisReply*)redisCommand(context,"LPUSH %s %s",key.c_str(),js.dump().c_str());
    if(reply==nullptr){
        Logger::instance().error("redis save offline file failed");
        return false;
    }
    freeReplyObject(reply);
    Logger::instance().info("save offline file key="+key);
    return 1;
}
vector<string> RedisManager::getOfflineFile(const string& username){
    vector<string> messages;
    if(redisContext_==NULL) return messages;
    redisContext* context=(redisContext*)redisContext_;
    string key="offline:file:"+username;

    redisReply* reply=(redisReply*)redisCommand(context,"LRANGE %s 0 -1",key.c_str());
    if(reply==NULL) return messages;

    for(size_t i=0;i<reply->elements;i++){
        if(reply->element[i]->str) messages.push_back(reply->element[i]->str);
    }
    freeReplyObject(reply);
    return messages;
}
void RedisManager::clearOfflineFiles(const string& username){
    if(redisContext_==nullptr) return;
    redisContext* context =(redisContext*) redisContext_;
    string key="offline:file:"+username;
    redisReply* reply =(redisReply*)redisCommand(context,"DEL %s",key.c_str());
    if(reply) freeReplyObject(reply);
}