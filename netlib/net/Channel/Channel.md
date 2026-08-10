Channel职责类比  
# 统一角色对应关系：
Epoller（epoll）= 医院监控大屏  
只负责感知：几号诊室有人呼叫（fd 产生事件），只报号码，不认识诊室、不知道谁看病。 

EventLoop = 总调度台（护士总台）  
拥有一张总登记表：诊室号 → 诊室负责人（fd→Channel）。  
工作：监控大屏报出诊室号，查表找到对应的诊室负责人，通知负责人自己处理呼叫。

Channel = 独立诊室  
每个诊室绑定一个诊室编号（fd），诊室内部存放：有人呼叫时要执行什么操作（回调函数：接诊新病人 / 读取病人诉求）。  
✅ 诊室只管自己内部业务，不需要拥有全院登记表。 

TcpServer / TcpConnection = 医生  
创建诊室，提前告诉诊室：有人呼叫你要干什么。 


Epoller是千里眼,发现有什么变动都汇报他的大哥EventLoop,EventLoop对一下自己的表,将对应的事交给对应的Channel处理 
  
# 具体流程:
epoll_wait 返回就绪fd  
        ↓  
EventLoop（调度台）拿到fd  
        ↓  
EventLoop 查询自己内部 unordered_map<int,Channel*>  
        ↓  
找到对应Channel  
        ↓  
Channel.handleEvent() → 执行内部回调  

# EnableReading()的作用
举个现实场景帮你感受区别  
场景：新开一间诊室 8  
你先安排好：有人呼叫，就让张医生接待（setReadCallback） 
但是你没通知监控室把 8 号诊室连上监控！ 
结果： 
病人在 8 号诊室大喊大叫（客户端发数据，内核产生 EPOLLIN） 
监控大屏根本看不见，护士永远收不到通知，永远不会触发张医生接诊。 
所以是告诉EventLoop   