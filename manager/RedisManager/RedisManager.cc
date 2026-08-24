#include "RedisManager.h"
#include <hiredis/hiredis.h>
#include "../../netlib/base/Logger/Logger.h"
using namespace std;
RedisManager& RedisManager::instance()
{
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
    if(redisContext_ != nullptr){
        redisContext* existing = (redisContext*)redisContext_;
        if(existing->err == 0) return true;
        redisFree(existing);
        redisContext_ = nullptr;
    }
    redisContext* context =redisConnect("127.0.0.1",6379);
    if(context==nullptr||context->err){
        LOG_ERROR<<"Redis连接失败";
        return false;
    }
    redisContext_=context;
    LOG_INFO<<"Redis连接成功";
    return true;
}
bool RedisManager::saveOfflineMessage(const std::string& username,const std::string& message){
    LOG_INFO<<"保存用户离线消息 用户="<<username;
    if(redisContext_==NULL) return 0;
    redisContext* context=(redisContext*)redisContext_;
    string key="offline:private:"+username;
    redisReply* reply=(redisReply*)redisCommand(context,"RPUSH %s %b",key.c_str(),message.data(),message.size());
    if(reply==NULL) return 0;
    freeReplyObject(reply);
    return 1;
}
vector<string> RedisManager::getOfflineMessage(const string& username){
    vector<string> messages;
    if(redisContext_==NULL) return messages;
    redisContext* context=(redisContext*)redisContext_;
    string key="offline:private:"+username;
    LOG_INFO<<"获取用户离线消息 key="<<key;
    redisReply* reply=(redisReply*)redisCommand(context,"LRANGE %s 0 -1",key.c_str());
    if(reply==NULL) return messages;
    LOG_INFO<<"获取离线消息数量="<<to_string(reply->elements);
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
    LOG_INFO<<"保存群聊离线消息 用户="<<username;
    if(redisContext_==NULL) return 0;
    redisContext* context=(redisContext*)redisContext_;
    string key="offline:group:"+username;
    redisReply* reply=(redisReply*)redisCommand(context,"RPUSH %s %b",key.c_str(),message.data(),message.size());
    if(reply==NULL) return 0;
    freeReplyObject(reply);
    return 1;
}
vector<string> RedisManager::getGroupOfflineMessage(const string& username){
    vector<string> messages;
    if(redisContext_==NULL) return messages;
    redisContext* context=(redisContext*)redisContext_;
    string key="offline:group:"+username;
    LOG_INFO<<"获取群聊离线消息 key="<<key;
    redisReply* reply=(redisReply*)redisCommand(context,"LRANGE %s 0 -1",key.c_str());
    if(reply==NULL) return messages;
    LOG_INFO<<"获取群聊离线消息数量="<<to_string(reply->elements);
    for(size_t i=0;i<reply->elements;i++){
        if(reply->element[i]->str) messages.push_back(reply->element[i]->str);
    }
    freeReplyObject(reply);
    return messages;
}
void RedisManager::clearGroupOfflineMessage(const string& username){
    if(redisContext_==nullptr) return;
    redisContext* context =(redisContext*)redisContext_;
    string key="offline:group:"+username;
    redisReply* reply =(redisReply*)redisCommand(context,"DEL %s",key.c_str());
    if(reply) freeReplyObject(reply);
}
bool RedisManager::setOnline(const string& username){
    if(redisContext_==nullptr) return 0;
    redisContext* context=(redisContext*) redisContext_;
    string key="online:"+username;
    redisReply* reply=(redisReply*)redisCommand(context,"SET %s 1 EX 30",key.c_str());
    if(reply==nullptr){
        LOG_ERROR<<"Redis设置在线状态失败";
        return false;
    }
    if(reply) freeReplyObject(reply);
    return 1;
}
bool RedisManager::setOffline(const string& username){
    if(redisContext_==nullptr)
        return false;
    redisContext* context=(redisContext*)redisContext_;
    string key = "online:" + username;
    LOG_INFO<<"设置用户离线 用户="<<username;
    redisReply* reply =(redisReply*)redisCommand(context,"DEL %s",key.c_str());
    if(reply == nullptr){
        LOG_ERROR<<"Redis删除在线状态失败";
        return false;
    }
    LOG_INFO<<"Redis删除在线状态成功 用户="+username;
    freeReplyObject(reply);
    return true;
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
    if(result){
        redisReply* ttlReply=(redisReply*)redisCommand(context,"TTL %s",key.c_str());
        if(ttlReply!=nullptr){
            if(ttlReply->type==REDIS_REPLY_INTEGER && ttlReply->integer<0){
                redisReply* expireReply=(redisReply*)redisCommand(context,"EXPIRE %s 15",key.c_str());
                if(expireReply!=nullptr) freeReplyObject(expireReply);
            }
            freeReplyObject(ttlReply);
        }
    }
    return result;
}
bool RedisManager::saveVerifyCode(const string& target,const string& code){
    if(redisContext_==nullptr) return 0;
    redisContext* context=(redisContext*)redisContext_;
    string key="verify:"+target;
    redisReply* reply=(redisReply*)redisCommand(context,"SET %s %s EX 300",key.c_str(),code.c_str());
    if(reply==nullptr) return false;
    freeReplyObject(reply);
    return 1;
}
string RedisManager::getVerifyCode(const string& target){
    if(redisContext_==nullptr) return "";
    redisContext* context=(redisContext*)redisContext_;
    string key="verify:"+target;
    LOG_INFO<<"Redis获取验证码 key="<<key;
    redisReply* reply=(redisReply*)redisCommand(context,"GET %s",key.c_str());
    if(reply==nullptr){
        LOG_ERROR<<"Redis返回结果为空";
        return "";
    }
    if(reply->type==REDIS_REPLY_STRING){
        string code=reply->str;
        LOG_INFO<<"Redis获取验证码成功";
        freeReplyObject(reply);
        return code;
    }
    LOG_ERROR<<"Redis返回验证码类型错误";
    freeReplyObject(reply);
    return "";
}
bool RedisManager::deleteVerifyCode(const string& target){
    if(redisContext_==nullptr) return 0;
    redisContext* context=(redisContext*)redisContext_;
    string key="verify:"+target;
    redisReply* reply=(redisReply*)redisCommand(context,"DEL %s",target.c_str());
    if(reply) freeReplyObject(reply);
    return 1;
}
bool RedisManager::saveOfflineFileRequest(const string& username,const json& js){
    if(redisContext_==nullptr) return 0;
    redisContext* context=(redisContext*)redisContext_;
    string key="offline:file:"+username;
    redisReply* reply=(redisReply*)redisCommand(context,"LPUSH %s %s",key.c_str(),js.dump().c_str());
    if(reply==nullptr){
        LOG_ERROR<<"Redis保存离线文件请求失败";
        return false;
    }
    freeReplyObject(reply);
    LOG_INFO<<"保存离线文件请求成功 key="<<key;
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
bool RedisManager::saveOfflineGroupInvite(const string& username,const json& js){
    if(redisContext_==nullptr) return false;
    redisContext* context=(redisContext*)redisContext_;
    string key="offline:group_invite:"+username;
    string data=js.dump();
    redisReply* reply=(redisReply*)redisCommand(context,"RPUSH %s %b",key.c_str(),data.data(),data.size());
    if(reply==nullptr){
        LOG_ERROR<<"Redis保存离线群邀请失败";
        return false;
    }
    freeReplyObject(reply);
    LOG_INFO<<"保存离线群邀请成功 key="<<key;
    return true;
}
vector<string> RedisManager::getOfflineGroupInvite(const string& username){
    vector<string> invites;
    if(redisContext_==nullptr) return invites;
    redisContext* context=(redisContext*)redisContext_;
    string key="offline:group_invite:"+username;
    redisReply* reply=(redisReply*)redisCommand(context,"LRANGE %s 0 -1",key.c_str());
    if(reply==nullptr) return invites;
    for(size_t i=0;i<reply->elements;i++){
        if(reply->element[i]->str) invites.push_back(reply->element[i]->str);
    }
    freeReplyObject(reply);
    return invites;
}
void RedisManager::clearOfflineGroupInvite(const string& username){
    if(redisContext_==nullptr) return;
    redisContext* context=(redisContext*)redisContext_;
    string key="offline:group_invite:"+username;
    redisReply* reply=(redisReply*)redisCommand(context,"DEL %s",key.c_str());
    if(reply) freeReplyObject(reply);
}