//文件版聊天室服务器
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/stat.h>  
#include <sys/types.h>

using namespace std;

/*借助好友.cpp的函数
userExists
isFriend
*/
bool userExists(const string& username){//用于在注册和登录前验证用户是否存在
    //1由用户名拼出完整路径
    string path="data/users/"+username+".txt";
    if(access(path.c_str(),F_OK)==0)
        return 1;
    return 0;
}
bool isFriend(const string& username,const string& friendname){//判断二人是否是好友 1==是,0==不是
    //1打开文件,不存在则自动创建
    string path="data/friends/"+username+".txt";
    int fd=open(path.c_str(),O_RDWR|O_CREAT,0644);
    char buf[1024*4];
    int n=read(fd,buf,sizeof(buf)-1);
    if(n<0){
        perror("read");
        close(fd);
        return 0;
    }
    buf[n]=0;
    close(fd);
    string s(buf);
    //2找到
    //if(s.find(friendname)!=string::npos) return 1; 问题:如果好友是bobb,寻找的是bob
    string line;
    //istringstream:istringstream iss(s)->把s按空格分割为iss.与getline搭配使用,实现读取一行并按空格拆分
    istringstream iss(s);
    while(getline(iss,line)){
        while(!line.empty()&&(line.back()=='\r'||line.back()=='\n'||line.back()==' ')) line.pop_back();
        if(line==friendname) return 1;
    }
    return 0;
}




string getHistoryFile(const string& a,const string& b){//寻找存a和b聊天记录的文件
    if(a<b) return "data/history/"+a+"_"+b+".txt";
    return "data/history/"+b+"_"+a+".txt";
}
bool sendPrivateMessage(const string& a,const string& b,const string& msg){//发起方a,接收方b,内容msg
    //1用户存在与否
    if(!(userExists(a)&&userExists(b))) return 0;
    //2是好友吗
    if(!isFriend(a,b)) return 0;
    //3打开文件,发送信息
    string history_file=getHistoryFile(a,b);
    int fd=open(history_file.c_str(),O_WRONLY|O_APPEND|O_CREAT,0644);
    if(fd<0){
        perror("open");
        return 0;
    }
    string s="[+a+]:"+msg+"\n";
    int n=write(fd,s.c_str(),s.size());
    if(n<0||n!=s.size()){
        perror("write");
        close(fd);
        return 0;
    }
    close(fd);
    return 1;
}
vector<string> getChatHistory(const string& a,const string& b){//返回私聊的聊天记录
    vector<string> ans;
    //1用户存在与否
    if(!(userExists(a)&&userExists(b))) return ans;
    //2是好友吗
    if(!isFriend(a,b)) return ans;
    //3找到聊天文件
    string history_file=getHistoryFile(a,b);
    int fd=open(history_file.c_str(),O_RDONLY,0644);
    if(fd<0){
        perror("open");
        return ans;
    }
    char buf[1024*4];
    int n=read(fd,buf,1024*4);
    if(n<0){
        perror("read");
        close(fd);
        return ans;
    }
    string s(buf,n);
    istringstream iss(s);
    string line;
    while(getline(iss,line)){
        while(!line.empty()&&line.back()=='\r'||line.back()=='\n'||line.back()==' ') line.pop_back();
        if(!line.empty()) ans.push_back(line);
    }
    return ans;
}
