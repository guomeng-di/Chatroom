# 为什么用Redis  
1> 假设你写了一个最简单的登录系统:  
```c++
unordered_map<string,string> users;

users["tom"]="123456";
users["jack"]="888888";

//登录时
string username,password;

if(users[username]==password)
{
    登录成功;
}
```
问题:当程序一关闭,数据就找不到了,因为 unordered_map只存在于内存 


2>于是想到,写到文件里,下次依旧可以找到阿    
问题:寻找时要从文件第一行开始一点一点读,太慢了  


3>于是,数据库产生   
**数据库的本质**就是:1帮你存数据 2帮你快速寻找数据  

## 连接方式
```
Tom客户端  
      |  
Jack客户端  
      |  
Rose客户端  
      |  
    Server  
      |  
    Redis  
```
这里:Redis就像仓库,Server像管理员  

>
演示具体流程:  
当客户端Tom想登录,发送消息*LOGIN tom 123456*给服务器  
服务器收到后,拿去问Redis要密码,进行核对  
密码正确,Tom登录成功  
全程,客户端只能与服务器交流,压根不知道Redis的存在  
>
## 数据存储分类  
### 第一类：长期数据（Redis）
```
账号
密码
好友关系
群成员
离线消息
聊天记录
```
服务器重启后还要存在  

### 第二类：运行时数据（内存）
```
fd
epoll_event
连接状态
接收缓冲区
发送缓冲区
```
服务器重启后本来就应该消失  

# TCP心跳检测 
客户端过一段时间冒个泡,证明自己还活着,没有出现突然*断网,拔网线,WiFi掉线,机器死机*等问题而导致离线  
怎么实现呢?  
想到epoll_wait中最后一个参数表示超时时间.此时,即使没有事情发生也会唤醒一次  

# Redis用法
string,hash(==结构体),set(元素不重复+查找快),list(消息队列)

## Hash
>
HSET  
HGET  
HGETALL  
HEXISTS  
>

## Set
>
SADD  
SREM  
SISMEMBER  
SMEMBERS  
>

## List
>
LPUSH/RPUSH  
LRANGE  
LPOP 
>

# HASh

1. HSET(hash set,给hash设置字段)  
eg:HSET user:tom password 123456  
user:tom  
↓  
这个Hash对象  
password  
↓  
字段名  
123456  
↓  
字段值  
相当于c++,user["tom"].password="123456";  

2. HGET(hash get,读取hash中某个字符段)  
eg:HGET user:tom password  
返回:123456  
相当于:cout<<user["tom"].password;  

3. HCETALL(hash get all,读取整个对象)  
eg:HGETALL user:tom  
返回:password 123456  
email tom@qq.com 
相当于:cout<<"user["tom"]";打印整个结构体   
4. RPUSH(right push,把消息追加到尾部) 
eg:RPUSH offline:jack hello
相当于:offline["jack"].push_back("hello");
5. HEXISTS(hash exists,判断hash字段是否存在)  
eg:HEXISTS user:tom   
意思:user中是否存在tom   
存在则注册失败,否则注册   

# SET 
相当于unordered_set  

1. SADD(set add,向集合中添加元素)
eg:SADD friends:tom jack  
相当于:friends["tom"].insert("jack"); 
2. SISMEMBER(set is member)  
eg:SISMEMBER friends:tom jack 
相当于:friends["tom"].find("jack");  
返回1,是好友;返回0,不是好友  
3. SMEMBERS(set members,获取整个集合)
eg:
有friends tom:  
jack  
rose  
Alice  
当: SMEMBERS friends:tom  
返回:  
jack  
rose  
Alice  
4. SREM(set remove,删除集合中的元素)  
eg:tom和jack删除好友时,需要:  
SREM friends:tom jack  
SREM friends:jack tom  
# LIST
#
对于聊天室,redis和c++中STL很相似,故可简单理解为  
Redis=网络版STL容器 
 





