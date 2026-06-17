# 1epoll  
## 1为什么要有epoll  
聊天室有多个客户端,而服务器需要同时处理如注册,登录多个功能   
最简单的,一个线程对应一个客户端  
但是这样会开多个线程,占用内存大,且cpu不断切换  
故想到,既然大部分socket都在等消息,可以让一个线程管理多个socket,于是:select,poll,epoll  
## 2理解epoll的思想
聊天室：
```
fd=5 Tom
fd=6 Jack
fd=7 Rose
fd=8 Alice
```
服务器想知道：  
谁发消息了？  
如果这样写：  
```
recv(5,...);
recv(6,...);
recv(7,...);
recv(8,...);
```
会阻塞。 
>为什么会阻塞呢  
好比你是老师,从第一排开始问同学,你有问题吗?  
当你问第一个同学时,他不吭声,你就只好一直等,导致根本轮不到后面有问题的同学  
同理,如果有socket的fd=5,6,7.6发来消息,你从5开始问,recv不到,你就一直等,导致阻塞
>

>为什么recv会一直等待呢  
因为设计recv的初衷是:你要接收数据,那我就一直等到数据传来  
accept连接客户端也是同理
>

Epoll：
```
Tom发消息
 ↓
Linux内核发现
 ↓
通知Epoll
 ↓
Epoll告诉服务器：
 fd=5有数据

服务器只处理：

recv(5,...);

不用遍历所有客户端。
```
## 3epoll的使用  
epoll的三大函数:  
epoll_create(),创建epoll对象得到epfd,可理解找到一个班的班主任   
epoll_ctl(),将socket交给epoll管理,相当于班主任记住学生名单  
epoll_wait(),等待socket事件,班主任等待同学问问题->返回发生的事件数n  
流程:  
```
socket
  ↓

bind
  ↓

listen
  ↓

epoll_create
  ↓

epoll_ctl(listen_fd)
  ↓

while(1)
{
    epoll_wait()

    有事件
       ↓

    处理事件
}
```
>为什么listen_fd也要加入epoll
因为accept也会阻塞,为防止卡死  
把 listenfd 加入 epoll   
有人连接  
↓  
listenfd可读  
↓  
epoll通知  
↓  
accept()  
这样不会阻塞  
>

>理解**阻塞socket**和**非阻塞socket**  
1. 阻塞socket  
int fd=accept(listen_fd,NULL,NULL);  
没有新的客户端连接时,就一直等待直到有客户端连接,即阻塞  
2. 非阻塞socket 
设置EWOULDBLOCK->意思:现在没有数据,稍后再试     
没有客户端连接,返回-1后,直接进行下一步  
>
正确逻辑:
```
while(1){
    int n=epoll_wait();
    for(...){
        if(fd==listen_fd){
            accept(...)
        }else{
            recv()
        }
    }
}
```
### epoll_create()  
```c++
int epfd=epoll_create(1);
//参数写1即可  
//epfd>=0->成功  =-1->失败
```
### epoll_event结构体
定义:
```
struct epoll_event
{
    uint32_t events;
    epoll_data_t data;
};
```
```c++
struct event ev;
ev.events=EPOLLLIN;//意思:监听可读事件
ev.data_fd=fd;//保存fd,说明这个事件属于fd,之后events[i].data_fd即可知道哪个fd发生了事件
```
>EPOLLIN的本质:  
**告诉epoll,你帮我盯着这个fd,当可读时告诉我**  
可读,也即这里有消息了,此时,这个fd现在执行**读**操作不会阻塞       
1. listen_fd
accept不会阻塞了  
 1. 队列里没有连接时,listen_fd不可读,所以accept会阻塞
 2. 队列里有连接时，listen_fd可读，所以accept不会阻塞
当客户端发出connect申请时,会将其加入listen_fd的等候队列,因为此时队伍里有连接了,所以accept时不会阻塞
2. client_fd
recv不会阻塞了  
  1. 客户端没有消息时,recv阻塞
  2. 有消息时,不阻塞
>
 
>sum:
EPOLLIN告诉epoll,你帮我看着这个fd,当他可读时通知我  
那么什么时候通知呢?  
对于不同 fd：  
listenfd  
↓  
连接队列非空  
↓  
accept可执行  
↓  
EPOLLIN    
clientfd  
↓  
接收缓冲区有数据  
↓  
recv可执行  
↓  
EPOLLIN     
 
此时fd可读,对应的才可以进行accept/recv操作
在此之前,执行时是会阻塞而导致有可能读不到想要的消息,所以,此时我们再执行这些操作  
>
### epoll_ctl()  
原型:
```
int epoll_ctl(
    int epfd,  ->哪个epoll
    int op,  ->操作类型,常见3个:EPOLL_CTL_ADD(添加fd),EPOLL_CTL_MOD(修改),EPOLL_CTL_DEL(删除)
    int fd,  ->要管理的fd
    struct epoll_event *event  ->监听什么事件
);
```
>EPOLL_CTL_MOD是在修改什么
修改ev.events  
例如,原来的ev.events=EPOLLIN;(监听可读事件)  
修改为 ev.events=EPOLLIN|EPOLLOUT(监听可读+可写事件)  
>

### epoll_wait()
原型:
```
int epoll_wait(
    int epfd,
    struct epoll_event *events,  ->将发生事件的fd存在数组中
    int maxevents,  ->数组最大容量
    int timeout  ->超时时间  -1表示一直等待
);
```
# 2LT(水平触发)/ET(边缘触发)  
1. 引入
已经理解了：  
EPOLLIN = fd处于可读状态  
那么问题来了：  
假设 fd 已经可读了  
缓冲区接收到:abcdefghij  
epoll 应该：  
只提醒你一次？  
还是一直提醒你？  
这就是LT和ET的区别  
2. LT(level Trigger)   
只要fd还满足条件，就一直通知  
eg:第一次recv读到abc->还没读完,就会继续epoll_wait()告诉你,还有数据->直到,**缓冲区清空** 
LT:  
有数据  
↓  
通知  
↓  
没读完  
↓  
继续通知  
↓  
读完  
↓  
停止  
3. ET(Edge Trigger)  
状态变化时通知一次  
客户端发送:abcdefghij  
缓冲区： 
空  
↓  
有数据  
发生变化： 
不可读  
    |  
    |  
    v  
可读  
这个瞬间：  
ET通知一次。  
就算没读完,状态也是可读->可读,所以不会启动epoll_wait()函数  
## ET为什么一定要清空缓冲区(循环读到缓冲区空)   
最初,缓冲区:abcdefg  
第一次从无到有,故会epoll_wait一次  
假设只recv了abc  
缓冲区还有:defg  
一直留在缓冲区,就算以后该客户端的缓冲区有了新发来的消息123  
ET也因为状态一直可读而不汇报  
因此,ET写法:  
```
while(true)
{
    int n = recv(fd, buf, sizeof(buf), 0);

    if(n > 0)
    {
        //处理数据
    }
    else if(n == -1 && errno == EAGAIN)
    {
        //缓冲区已经空了
        break;
    }
    else
    {
        //客户端关闭或出错
        break;
    }
}
```



