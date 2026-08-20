 client:

 主线程:   main thread
             |
        输入菜单
             |
          send()
负责用户操作：
1 chat
2 friend list
3 group


接收线程:  recv thread
             |
        recv()
             |
        根据msgid处理
负责服务器主动消息：
例如：
好友申请
好友上线
聊天消息
离线消息
群消息


             client

        +-------------+
        |             |
        |  主线程     |
        |             |
        | 输入命令    |
        | send()      |
        |             |
        +-------------+

              |
              |
              | socket
              |
              |

        +-------------+
        |             |
        | 接收线程    |
        |             |
        | recv()      |
        | 分发消息    |
        |             |
        +-------------+



现在:
client.cc

main线程
 |
 |-- 输入命令
 |-- 构造JSON
 |-- send()

recv线程
 |
 |-- recv()
 |-- json解析
 |-- 根据msgid处理

以后:
client
 |
 |-- client.cc
 |
 |-- ClientNetwork
 |       |
 |       |--send()
 |       |--recv()
 |
 |-- ClientHandler
         |
         |--处理CHAT_NOTIFY
         |--处理GROUP_NOTIFY
         |--处理FRIEND_NOTIFY




心跳检测:
                 client
                   │
          ┌────────┴────────┐
          │                 │
       主线程             recv线程
          │                 │
     select/poll          recv()
          │                 │
    ┌─────┴──────┐          │
    │            │          │
  stdin       timerfd      │
    │            │          │
 用户命令      5秒到期       │
    │            │          │
    │       HEARTBEAT_MSG   │
    │            │          │
    └──────┬─────┘          │
           │                │
           └──── send() ────┘