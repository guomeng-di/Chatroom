#include <bits/stdc++.h>
#include <thread>
#include <signal.h>  
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>

using namespace std;

int ctrl_fd=-1;
int data_fd=-1;

void sendCommand(int fd,const string& s){
    string s1=s+"\r\n";
    send(fd,s1.c_str(),s1.size(),0);
    return ;
}
string recvResponse(int fd){
    char buf[1024*4]={0};
    int n=recv(fd,buf,1024*4,0);
    if(n<=0) return "";
    return string(buf,n);
}
bool login(){
    sendCommand(ctrl_fd,"USER A");
    cout<<recvResponse(ctrl_fd);
    sendCommand(ctrl_fd,"PASS 123");
    cout<<recvResponse(ctrl_fd);
    return 1;
}
bool connectServer(){//连接服务器,用于发送list等命令
   ctrl_fd=socket(AF_INET,SOCK_STREAM,0);
   sockaddr_in addr;
   addr.sin_family=AF_INET;
   addr.sin_port=htons(2100);
   inet_pton(AF_INET,"127.0.0.1",&addr.sin_addr);
   connect(ctrl_fd,(sockaddr*)&addr,sizeof(addr));
   cout<<recvResponse(ctrl_fd);
   if(login) return 1;
}
void enterPasv(){//开启被动模式,建立新的通道
    sendCommand(ctrl_fd,"PASV");
    string s=recvResponse(ctrl_fd);
    cout<<s;
    int h1,h2,h3,h4,p1,p2;
    //"227 Entering Passive Mode(127,0,0,1,"+ to_string(p1) + "," + to_string(p2) + ")\r\n"
    sscanf(s.c_str(),"227 Entering Passive Mode(%d,%d,%d,%d,%d,%d)",&h1,&h2,&h3,&h4,&p1,&p2);
    int port=p1*256+p2;

    data_fd=socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in addr;
    addr.sin_family=ntohs(port);
    addr.sin_family=AF_INET;
    inet_pton(AF_INET,"127.0.0.1",&addr.sin_addr);
    connect(data_fd,(sockaddr*)&addr,sizeof(addr));
    return;
}


void list_(){
    enterPasv();
    sendCommand(ctrl_fd,"LIST\r\n");
    cout<<recvResponse(ctrl_fd);
    char buf[1024*4]={0};
    int n;
    while(n=recv(data_fd,buf,1024*4,0)>0) cout.write(buf,n);
    close(data_fd);
    cout<<recvResponse(ctrl_fd);
    return ;
}
int main(){
    while(1){
        cout<<"===== FTP CLIENT =====\n1. LIST\n2. GET\n3. PUT\n4. QUIT\n\nchoice:";
        int N; cin>>N;
        if(N==1) list_();
        else if(N==2) get_();
        else if(N==3) put_();
        else if(N==4) {quit_();break;}
        else continue;
    }
    return 0;
}