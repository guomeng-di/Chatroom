// 借助 libcurl 网络库，走 QQ 邮箱 SMTP 加密协议，把一段验证码文本打包成一封标准邮件
// 上传到 QQ 邮箱服务器，由服务器转发给目标收件邮箱。
// 类比：curl 就是快递员，QQ SMTP 服务器是快递中转站，我们把信件交给中转站
// 中转站负责投递到对方邮箱
#include "EmailManager.h"
#include "../../netlib/base/Logger.h"
#include <iostream>
#include <curl/curl.h>
#include <cstring>
using namespace std;
//回调函数:SMTP 发邮件是分段上传
static size_t payloadSource(
    void* ptr,//存本次要上传的邮件片段
    size_t size,
    size_t nmemb,
    void* userdata)//完整信件内容
{
    string* mail = static_cast<string*>(userdata);
    size_t maxLen = size * nmemb;//计算本次最多能传输多少字节
    size_t len = mail->size();

    if(len > maxLen) len = maxLen;

    memcpy(ptr, mail->c_str(), len);
    mail->erase(0,len);
    return len;
}

EmailManager& EmailManager::instance(){
    static EmailManager manager;
    return manager;
}
bool EmailManager::sendCode(const std::string& email,const std::string& code){
    //句柄初始化,后续通过它进行发送操作
    CURL* curl=curl_easy_init();
    //初始化失败
    if(!curl){
        Logger::instance().error("curl init failed");
        return 0;
    }
    string username="3646455676@qq.com";
    string password="yqscfeoyoyqdciaf";
    string mail="To: "+email+"\r\n"
        "From: "+username+"\r\n"
        "Subject: ChatRoom Verify Code\r\n"
        "\r\n"
        "Your verify code is: "+ code + "\r\n"
        "\r\n";

        // ==========新增SSL强制配置==========
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    // ===================================
    
    //发件人准备:
        //设置网络访问地址
        curl_easy_setopt(curl,CURLOPT_URL,"smtps://smtp.qq.com:465");
        //配置SMTP登录账号密码,用来和QQ邮箱服务器做身份校验，校验不通过直接拒绝发信
        curl_easy_setopt(curl,CURLOPT_USERNAME,username.c_str());
        curl_easy_setopt(curl,CURLOPT_PASSWORD,password.c_str());

        curl_easy_setopt(
    curl,
    CURLOPT_LOGIN_OPTIONS,
    "AUTH=LOGIN"
);

        //指定发件人地址
        curl_easy_setopt(curl,CURLOPT_MAIL_FROM,username.c_str());

    //收件人准备:
        //email要送达的地址用curl_slist链表存储，这里先定义链表头，初始化为空。
        struct curl_slist* recipients=nullptr;
        //往收件人链表追加一个邮箱地址
        recipients=curl_slist_append(recipients,email.c_str());
        //告诉curl发送的地址
        curl_easy_setopt(curl,CURLOPT_MAIL_RCPT,recipients);
        //开启上传模式：SMTP 发邮件本质是把邮件报文上传到邮箱服务器，必须开启这个选项
        curl_easy_setopt(curl,CURLOPT_UPLOAD,1L);


        //告诉curl需要上传的内容
        curl_easy_setopt(curl,CURLOPT_READDATA,&mail);
        //设置读取回调
        curl_easy_setopt(curl,CURLOPT_READFUNCTION,payloadSource);
        // 关闭SSL校验
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(
    curl,
    CURLOPT_VERBOSE,
    1L
);
        //正式发送
        CURLcode res=curl_easy_perform(curl);
        //释放
        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);

        if(res!=CURLE_OK){
            Logger::instance().error("send email failed:"+string(curl_easy_strerror(res)));
            return 0;
        }
        Logger::instance().info("send email success");
        return 1;
}