# 1文件操作
```c++
open
read
write
close
lseek
```
## open

int fd=open(文件名,打开方式);

```c++
int fd=open("data.txt",O_RDONLY);
if(fd<0){
perror("open");
return;
}
```
打开方式复习:

O_RDONLY:only read 只读

O_WRONLY:only write 只写

## read
### 简单文件读取:
```
int fd=open("data.txt",O_RDONLY);
if(fd<0){
perror("open");
return;
}
char buf[1024*4];
int n=read(fd,buf,sizeof(buf));
buf[n]='\0';
close(fd);

\\打开文件,fd此后代指文件,读取文件内容记入buf中
```
### 大文件读取:
```c++
int fd=open("data.txt",O_RDONLY);
if(fd<0){
perror("open");
return;
}
char buf[1024*4];

while(1){
int n=read(fd,buf,sizeof(buf));
if(n<0){
perror("read");
break;
}
if(n==0)  break;
cout.write(buf,n);
}
close(fd);
```


## write
### 1写文件
```c++
int fd=open("log.txt",O_WRONLY|O_CREAT|O_APPEND,0666);
string s="user login\n";
write(fd,s.c_str(),s.size());
```
### 2socket
服务器:
```c++
write(client_fd,"hello",5);
```
则,客户端收到hello

## lseek
移动文件指针的位置
```c++
 lseek(int fd, 偏移几个位置, 从哪里开始偏移);
```
从哪里开始偏移,常见的有:

SEEK_SET 从头

SEEK_CUR 从当前位置

SEEK_END 从文件末尾(即:追加)

## open,read,write,close使用(最小聊天室服务器端示例)

### 程序功能概述
启动服务器监听端口

接受客户端连接

客户端发送消息

服务器：

打印消息到控制台

保存到 chat.log

客户端可以输入 /history 查看聊天记录

### 代码:
```c++
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>

using namespace std;

#define PORT 8888;//端口号
#define BACKLOG 5//最多listen个数

//写日志函数
void writeLog(const string& s){
    int fd=open("chat.log",O_WRONLY|O_CREAT|O_APPEND,0666);
    if(fd<0){
        perror("open log");
        return ;
    }
    write(fd,s.c_str(),s.size());
    close(fd);
}

//读取历史记录函数
void readHistory(int client_fd){
    int fd=open("chat.log",O_RDONLY);
    if(fd<0){
        string s="没有历史消息哦!\n";
        write(client_fd,s.c_str(),s.size());
        return ; 
    }
    char buf[1024*4];
    int n;
    while((n=read(fd,buf,sizeof(buf)))>0){
        write(client_fd,buf,n);
    }
    close(fd);
}
int main(){
    //1socket
    int server_fd=socket(AF_INET,SOCK_STREAM,0);
    if(server_fd<0){
        perror("socket");
        return 1;
    }
    //2bind
    sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port=htons(8888);
    addr.sin_addr.s_addr=INADDR_ANY;

    if(bind(server_fd,(sockaddr*)&addr,sizeof(addr))){
        perror("bind");
        return 1;
    }
    //3listen
    if(listen(server_fd,BACKLOG)<0){
        perror("listen");
        return 1;
    }
    cout<<"服务器启动"<<endl;
    //客户端
    while(1){
        sockaddr_in client_addr;
        socklen_t client_len=sizeof(client_addr);
        int client_fd=accept(server_fd,(sockaddr*)&client_addr,&client_len);
        if(client_fd<0){
            perror("accept");
            return -1;
        }
        cout<<"客户端连接成功"<<endl;

        char buf[1024*4];
        int n;
        while((n=read(client_fd,buf,sizeof(buf)))>0){
            buf[n]='\0';
            string s(buf);//s=buf

            if(s=="/history\n"){
                readHistory(client_fd);
            }else{
                writeLog(s);
                write(client_fd,s.c_str(),s.size());
            }
        }
        close(client_fd);
    }
    close(server_fd);
    return 0;
}
```
#### accept函数补充:

int fd=accept(fd,客户端地址addr,地址长度);
```c++
sockaddr_in client_addr;
socklen_t client_len = sizeof(client_addr);
int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
//创建好客户端的地址,accept时客户端会自动传给服务器
```
常用accept(fd,NULL,NLL)是因为服务器不关心客户端的ip地址和端口号
