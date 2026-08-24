#include "VerifyCodeService.h"
#include "../../manager/RedisManager/RedisManager.h"
#include "../../manager/EmailManager/EmailManager.h"
#include "../../utils/VerifyCode/VerifyCode.h"
#include "../../protocol/MsgId.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"

#include "../../netlib/base/Logger/Logger.h"
using namespace std;
json VerifyCodeService::sendCode(const json& js){
    json res;
    res["msgid"]=SEND_VERIFY_CODE_ACK;
    if(!js.contains("email")){
        LOG_ERROR<<"获取验证码缺少邮箱";
        res["errno"]=1;
        res["message"]="email empty";
        return res;
    }
    string email=js["email"];
    //生成验证码
    string code=VerifyCode::generate();
    LOG_INFO<<"生成验证码:"<<code;

    //redis保存
    if(!RedisManager::instance().connect()){ 
        LOG_ERROR<<"Redis连接失败";
        res["errno"]=1;
        res["message"]="send verify code failed";
        return res;
    }
    RedisManager::instance().saveVerifyCode(email,code);
    //发送给客户端邮箱
    EmailManager::sendCode(email,code);

    res["errno"]=0;
    res["message"]="send verify code success";

    return res;
}