//使用 C++ 实现一个单线程 Epoll 服务器
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
using namespace std;
#define PORT 8888
int main(){
    //1socket
    int listen_fd=socket(AF_INET,SOCK_STREAM,0);
    if(listen_fd<0){
        perror("socket");
        return -1;
    }
    //2bind
    sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port=htons(PORT);
    addr.sin_addr.s_addr=INADDR_ANY;
    bind(listen_fd,(sockaddr*)&addr,sizeof(addr));
    //3listen
    listen(listen_fd,10);
    //4epoll_create
    int epfd=epoll_create(1);
    if(epfd==-1){
        perror("epoll_create");
        return -1;
    }
    //5epoll_event
    struct epoll_event ev;
    ev.events=EPOLLIN;
    ev.data.fd=listen_fd;
    //6epoll_ctl
    epoll_ctl(epfd,EPOLL_CTL_ADD,listen_fd,&ev);
    //7epoll_wait
    while(1){
        struct epoll_event events[1024];
        int n=epoll_wait(epfd,events,1024,-1);
        for(int i=0;i<n;i++){
            if(events[i].data.fd==listen_fd){//监听端汇报:有新的client_fd来啦
                int client_fd=accept(listen_fd,NULL,NULL);
                if(client_fd<0){
                    perror("accept");
                    return -1;
                }
                //epoll_event
                struct epoll_event ev1;
                ev1.data.fd=client_fd;
                ev1.events=EPOLLIN;
                epoll_ctl(epfd,EPOLL_CTL_ADD,client_fd,&ev1);
            }else{//客户端的消息
                char buf[1024];
                int n=recv(events[i].data.fd,buf,1024,0);
                if(n<0){
                    perror("recv");
                    return -1;
                }else if(n==0){
                    //epoll_ctl
                    epoll_ctl(epfd,EPOLL_CTL_DEL,events[i].data.fd,NULL);
                    close(events[i].data.fd);
                    cout<<"client quit";
                    continue;
                }else{
                    send(events[i].data.fd,buf,n,0);
                }
            }
        }
    }
    return 0;
}