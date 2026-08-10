protocol/

├── MsgId 消息类型编号,负责 分类
├── JsonProtocol JSON转换工具
├── Message 保存消息(长度+内容)
└── MessageCodec 加长度/去长度 

1 MsgId 
它解决的问题：  
收到一条消息后，服务器怎么知道它是什么请求？  
它给客户端登录注册等不同类型的消息加上编号，便于分类  

2 JsonProtocol  
JSON转换工具.实现消息的序列化和反序列化  

3 Message  
表示一条完整消息的数据结构.保存:一条消息的长度+内容  

4 MessageCodec  
对于要发送的消息,加长度再发送  
对于接收的消息,去掉长度  

串一下:  
比如登录:  
产生:{msgid:1,name:"jack"}  
 |
 v
JsonProtocol转为JSON字符串  
 |
 v
Message保存消息  
 |
 v
处理后的消息MessageCodec给它加长度  
 |
 v
TCP send()  
