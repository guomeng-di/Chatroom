# 负责：
生成验证码
保存验证码
验证验证码



# 流程:
client.cc
    |
    | 发送验证码请求
    ↓

MessageDispatcher
    |
    | 分发msgid
    ↓

VerifyCodeService
    |
    | 生成验证码
    | 存Redis
    ↓

Redis

然后：

client recvMessage

收到验证码响应

显示

