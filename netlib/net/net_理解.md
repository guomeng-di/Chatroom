# 完整流程:
饭店准备开业：
先安排一名调度专员 EventLoop，循环等待各类消息；调度专员手里有监控设备 Epoller，专门监测各个房门动静。

聘请 TcpServer 饭店总管（负责后厨），总管雇佣保安 Acceptor 驻守大门。

保安入职,记下来自己做什么工作.保安制作一张 Channel 任务卡片：大门口有人来访，我执行 accept 迎接客人。但保安记性太差了,就交给了调度员.调度专员把大门交给 Epoller 持续监控。

客人来到饭店大门口：
Epoller 监测大门产生事件，上报调度专员 EventLoop。
EventLoop 找到大门对应的 Channel 卡片，执行对应工作,也就是:保安 Acceptor 出面接待客人，分配房间编号client_fd。

保安通知总管 TcpServer 新客人到达，TcpServer安排一名专属服务员 TcpConnection，一对一服务这位客人。

服务员制作自己房间的 Channel 卡片：房间客人说话（可读），我就负责接收消息。但服务员记性太差了,就交给了调度员.

总管把【房间号 client_fd + 服务员 conn】记录在自己的花名册 unordered_map 里。

EventLoop 通知 Epoller：持续监控这间客房。

客人点菜，发送消息,Epoller 监测客房 fd 可读，上报 EventLoop。

EventLoop 查询这间房的 Channel，通知专属服务员 TcpConnection。

服务员调用 recv 拿到客人消息，交给上层业务代码处理（解析点餐 / 聊天指令）。->菜单给厨师

业务处理完成，通知服务员，服务员调用 send 把回复送给客人。->菜送回来

客人用餐完毕准备离开,服务员 recv 返回 0，发现客人下线。

服务员通知总管 TcpServer,总管在花名册 map 中将这条记录划掉。

总管对服务员 TcpConnection说你走吧,不用干了(释放TcpConnection)。

EventLoop 通知 Epoller：不需要继续监控这个房间 fd。
# Epoller
1. addFd(fd, events)  
新增一扇房门，交给监视器持续监视  
底层调用 epoll_ctl(EPOLL_CTL_ADD)  

2. removeFd(fd)  
客人走了，把这扇房门从监视器里移除，不再监控  
底层调用 epoll_ctl(EPOLL_CTL_DEL)  

3. modifyFd (fd, events)  
翻译：修改房门的监控规则  
场景举例：  
一开始我们只监视房门【可读】（客户端发消息）。  
之后服务器要主动给客户端发大量数据，缓冲区满了，我们要监听【可写】；  
发送完成之后，又不需要监听可写了。 
底层调用 epoll_ctl(EPOLL_CTL_MOD)  

4. wait()  
监视器持续等待，直到有房门出现事件。  
返回值：一共有多少个房门发生了事件。  

5. getEventFd(int index)  
wait 之后，监视器拿到一堆 “有动静的房间清单”。  
index 就是清单下标。   
getEventFd(i)：取出第 i 个发生事件的房门编号 fd。 

6. getEvents(int index)  
取出第 i 个房间，发生了什么类型的事件：  
是客人发消息 (EPOLLIN)？还是可以发数据(EPOLLOUT)？或是连接断开？  

# EventLoop
1. loop()  
不断问有没有事  
有事时,Epoller会告诉EventLoop,这时EventLoop拿出卡片,记录下本次发生的事revents,并告诉Acceptor/TcpConnection执行预先在卡片写好的事件  

2. addChannel()  
Channel保存到EventLoop中  

3. updateChannel()  
同步信息到 Epoller，修改 epoll 的监听规则  
如果这个 Channel还没有加入 Epoller 监控  
→ updateChannel 内部调用 epoller_->addFd()  
如果这个 Channel已经在 Epoller 监控当中  
→ updateChannel 内部调用 epoller_->modifyFd()  

# Channel  
Channel保存:fd+events+revents+回调函数  
调度员需要从这里知道fd和events  
有新的事件发生(revents)则setRevents()  
在events记录后,判断执行哪个函数,卡片上写下这个函数  
判断的过程是handleEvent()  
记下的回调函数是setRead/Write/Close Callback  

# Acceptor
Acceptor是保安,负责迎接客人  
当Acceptor被聘用时,它记录下TcpServer的电话,有客人来了就打电话 void setNewConnectionCallback(
        std::function<void(int)> cb
    );  

start: 保安出门开始工作,等待客人  
handleRead: 客人来了,保安先迎接(accept),再打电话     

# TcpConnection 
是客人坐下后专门负责客人一切的  
包括:handleRead(),send(),handleClose()  


