#include "ResetPasswordService.h"

#include "../../manager/RedisManager/RedisManager.h"
#include "../../model/UserModel/UserModel.h"
#include "../../utils/SHA256/SHA256.h"
#include "../../protocol/MsgId.h"
#include "../../netlib/base/Logger.h"
using namespace std;
json ResetPasswordService::resetPassword(const json& js){
    json res;
    res["msgid"]=RESET_PASSWORD_ACK;
    if(!js.contains("email")||!js.contains("password")||!js.contains("code")){
        Logger::instance().error("reset password lack params");
        res["errno"]=1;
        res["message"]="lack params";
        return res;
    }
    
    string email=js["email"];
    string code=js["code"];
    string password=js["password"];
Logger::instance().info("reset email="+email);

Logger::instance().info("reset code="+code);
    //1验证验证码
    if(!RedisManager::instance().connect()){
        Logger::instance().error("redis connect failed");
        res["errno"]=1;
        res["message"]="redis connect failed";
        return res;
    }
    string right_code=RedisManager::instance().getVerifyCode(email);

    Logger::instance().info("email="+email);
    cout<<"email="<<email<<endl;
    Logger::instance().info("input code="+code);
    cout<<"input code="<<code<<endl;
    Logger::instance().info("redis code="+right_code);
    cout<<"redis code="<<right_code<<endl;

    if(right_code!=code){
        Logger::instance().error("code wrong");
        res["errno"]=1;
        res["message"]="code wrong";
        return res;
    }
    //2验证码正确->修改密码
    string passwordHash=HashSHA256::encode(password);
    UserModel model;
    //根据邮箱找用户名
    string username=
    model.queryUsernameByEmail(email);
    if(username.empty()){
        Logger::instance().error("username not exist");
        res["errno"]=1;
        res["message"]="username not exist";
        return res;
    }
    if(!model.updatePasswordByEmail(email,passwordHash)){
        Logger::instance().error("update password failed");
        res["errno"]=1;
        res["message"]="update password failed";
        return res;
    }
    Logger::instance().info("update password success");
    res["errno"]=0;
    res["message"]="update password success";
    return res;
}