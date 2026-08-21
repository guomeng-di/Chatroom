#include "RegisterService.h"
#include "../../protocol/MsgId.h"
#include "../../model/UserModel/UserModel.h"
#include <iostream>
#include "../../utils/SHA256/SHA256.h"
#include "../../netlib/base/Logger.h"
#include "../../manager/RedisManager/RedisManager.h"
using namespace std;
RegisterService::RegisterService(){

}
RegisterService::~RegisterService(){

}
json RegisterService:: registerUser(const json& js){
    json res;
    res["msgid"]=REGISTER_ACK;

    //1参数检查
    if(!js.contains("username")||!js.contains("password")||!js.contains("email")||!js.contains("code")){
        Logger::instance().error("lack params");
        res["errno"]=1;
        res["message"]="lack params";
        return res;
    }
    //1获取客户端传来的账号密码
    string username=js["username"];
    string password=js["password"];
    string email=js["email"];
    string inputCode=js["code"];

    //2检查用户名是否已经存在
    UserModel model;
    if(model.queryUserByUsername(username)){
        Logger::instance().error(username+" already exists");
        res["errno"]=1;
        res["message"]="register failed,user already exists";
        return res;
    }

    //3对比验证码
    if(!RedisManager::instance().connect()){
    Logger::instance().error("redis connect failed");
    res["errno"]=1;
    res["message"]="redis connect failed";

    return res;
}
    string realCode=RedisManager::instance().getVerifyCode(email);
    if(inputCode!=realCode){
        res["errno"]=1;
        res["message"]="code wrong";
        return res;
    }

    //4密码加密
    string passwordHash = HashSHA256::encode(password);
    Logger::instance().info("register request username="+username);
    //5数据库存入(模拟)
    bool flag=model.insertUser(username,passwordHash,email);
    //存入成功
    if(flag){
        Logger::instance().info(username+" register success");
        res["errno"]=0;
        res["message"]="register success";
    }else{
        Logger::instance().error(username+" register failed,user already exists");
        res["errno"]=1;
        res["message"]="user already exists";}
   //返回结果
   return res;
}