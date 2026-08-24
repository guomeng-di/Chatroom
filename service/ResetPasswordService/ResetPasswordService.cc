#include "ResetPasswordService.h"

#include "../../manager/RedisManager/RedisManager.h"
#include "../../model/UserModel/UserModel.h"
#include "../../utils/SHA256/SHA256.h"
#include "../../protocol/MsgId.h"
#include "../../netlib/base/Logger/Logger.h"
using namespace std;
json ResetPasswordService::resetPassword(const json& js){
    json res;
    res["msgid"]=RESET_PASSWORD_ACK;
    if(!js.contains("email")||!js.contains("password")||!js.contains("code")){
        LOG_ERROR<<"重置密码请求缺少参数";
        res["errno"]=1;
        res["message"]="lack params";
        return res;
    }
    
    string email=js["email"];
    string code=js["code"];
    string password=js["password"];
LOG_INFO<<"重置密码邮箱="<<email;

LOG_INFO<<"重置密码验证码="<<code;
    //1验证验证码
    if(!RedisManager::instance().connect()){
        LOG_ERROR<<"Redis连接失败";
        res["errno"]=1;
        res["message"]="redis connect failed";
        return res;
    }
    string right_code=RedisManager::instance().getVerifyCode(email);

    LOG_INFO<<"邮箱="<<email;
    LOG_INFO<<"输入验证码="<<code;
    LOG_INFO<<"Redis验证码="<<right_code;

    if(right_code!=code){
        LOG_ERROR<<"验证码错误";
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
        LOG_ERROR<<"用户名不存在";
        res["errno"]=1;
        res["message"]="username not exist";
        return res;
    }
    if(!model.updatePasswordByEmail(email,passwordHash)){
        LOG_ERROR<<"修改密码失败";
        res["errno"]=1;
        res["message"]="update password failed";
        return res;
    }
    LOG_INFO<<"修改密码成功";
    res["errno"]=0;
    res["message"]="update password success";
    return res;
}