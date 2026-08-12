#include "VerifyCodeService.h"
#include "../../manager/RedisManager/RedisManager.h"
#include "../../manager/EmailManager/EmailManager.h"
#include "../../utils/VerifyCode/VerifyCode.h"
#include "../../protocol/MsgId.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"

#include "../../netlib/base/Logger.h"
using namespace std;
json VerifyCodeService::sendCode(const json& js){
    json res;
    res["msgid"]=SEND_VERIFY_CODE_ACK;
    if(!js.contains("email")){
        Logger::instance().error("verify code lack email");
        res["errno"]=1;
        res["message"]="email empty";
        return res;
    }
    string email=js["email"];
    //生成验证码
    string code=VerifyCode::generate();
    Logger::instance().info("verify code="+code);

    //redis保存
    if(!RedisManager::instance().connect()){ 
        Logger::instance().error("redis connect failed");
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
