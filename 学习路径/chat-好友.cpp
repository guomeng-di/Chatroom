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

bool createDir(const string& s){
    if(access(s.c_str(),F_OK)==-1){//access判断是否存在,==0已存在 ==-1表示不存在
        if(mkdir(s.c_str(),0755)==-1){//创建但失败
            perror("mkdir:s.c_str()");
            return 0;
            }
    }
    return 1;
}
void initDataDirs(){//测试能否自动创建目录
    createDir("data");
    createDir("data/users");//存放用户账号信息
    createDir("data/friends");//存放好友关系
    createDir("data/groups");//存放群组消息
    createDir("data/history");//历史记录
    createDir("data/offline");//离线信息
    createDir("data/logs");//服务器日志
    return ;
}

bool userExists(const string& username){//用于在注册和登录前验证用户是否存在
    //1由用户名拼出完整路径
    string path="data/users/"+username+".txt";
    if(access(path.c_str(),F_OK)==0)
        return 1;
    return 0;
}

bool registerUser(const string& username,const string& password){//注册
    //1注册前判断用户是否存在
    if(userExists(username))//存在
     return 0;
    //2不存在->创建文件
    string path="data/users/"+username+".txt";
    int fd=open(path.c_str(),O_WRONLY|O_CREAT |O_EXCL,0644);
    if(fd<0){
        perror("open");
        return 0;
    }
    //3把密码写入文件
    int n=write(fd,password.c_str(),password.size());
    if(n<0||n!=password.size()){
        perror("write");
        close(fd);
        return 0;
    }
    close(fd);
    return 1;
}

bool loginUser(const string& username,const string& password){//登录
    //1登录前判断用户是否存在
    if(!userExists(username)) return 0;
    //2登录:密码比较
    //2-1拼接路径
    string path="data/users/"+username+".txt";
    //2-2打开,密码写入buf,转存入s
    int fd=open(path.c_str(),O_RDONLY,0644);
    if(fd<0){
        perror("open");
        return 0;
    }
    char buf[128];
    int n=read(fd,buf,sizeof(buf)-1);
    if(n<0){
        perror("read");
        close(fd);
        return 0;
    }
    buf[n]=0;
    close(fd);
    string s(buf);
    //2-3清理空格
    while(!s.empty()&&(s.back()=='\r'||s.back()==' '||s.back()=='\n')) s.pop_back();
    //2-4比较密码
    return s==password;
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
bool addFriend(const string& username,const string& friendname){//添加好友
    //1检查用户是否存在
    if(!(userExists(username)&&userExists(friendname))) return 0;
    //2检查是不是本人
    if(username==friendname) return 0;
    //3检查两人是不是朋友
    if(isFriend(username,friendname)) return 0;
    //4添加friendname到username.txt
    string path="data/friends/"+username+".txt";
    int fd=open(path.c_str(),O_WRONLY|O_APPEND|O_CREAT,0644);
    if(fd<0){
        perror("open");
        return 0;
    }
    string s=friendname+"\n";
    int n=write(fd,s.c_str(),s.size());
    if(n<0||n!=s.size()){
        perror("write");
        close(fd);
        return 0;
    }
    close(fd);
    //5
    string path1="data/friends/"+friendname+".txt";
    int fd1=open(path1.c_str(),O_WRONLY|O_APPEND|O_CREAT,0644);
    if(fd1<0){
        perror("open");
        return 0;
    }
    string s1=username+"\n";
    int n1=write(fd1,s1.c_str(),s1.size());
    if(n1<0||n1!=s1.size()){
        perror("write");
        close(fd1);
        return 0;
    }
    close(fd1);
    return 1;
}

vector<string> listFriends(const string& username){//列出朋友
    vector<string> res;
    //1检查用户存在与否
    if(!userExists(username)) return res;
    //2拼接路径,打开目录
    string path="data/friends/"+username+".txt";
    int fd=open(path.c_str(),O_RDONLY,0644);
    if(fd<0){
        perror("open");
        return res;
    }
    char buf[1024*4];
    int n=read(fd,buf,sizeof(buf)-1);
    if(n<0){
        perror("read");
        close(fd);
        return res;
    }
    buf[n]=0;
    close(fd);
    string s(buf);
    istringstream iss(s);
    string line;
    while(getline(iss,line)){

        while(!line.empty()&&(line.back()=='\n'||line.back()=='\r'||line.back()==' ')){
            line.pop_back();
        }
        if(!line.empty()) res.push_back(line);
    }
    return res;
}

bool removeFriend(const string&username,const string&friendname){
    //1检查两人存在与否
    if(!(userExists(username)&&userExists(friendname))) return 0;
    //2检查两人是不是好友
    if(!isFriend(username,friendname)) return 0;
    //3列出朋友
    vector<string> l=listFriends(username);
    //4打开username所在文件夹,写入删除friendname后的好友名
    string path="data/friends/"+username+".txt";
    int fd=open(path.c_str(),O_WRONLY|O_TRUNC,0644);//O_TRUNC清空写入
    if(fd<0){
        perror("open");
        return 0;
    }
    for(int i=0;i<l.size();i++){
        if(l[i]!=friendname){
            string s=l[i]+"\n";
            int n=write(fd,s.c_str(),s.size());
            if(n<0&&n!=s.size()){
                perror("write");
                close(fd);
                return 0;
            }
        }
    }close(fd);
    //
    vector<string> l1=listFriends(friendname);
    string path1="data/friends/"+friendname+".txt";
    int fd1=open(path1.c_str(),O_WRONLY|O_TRUNC,0644);//O_TRUNC清空写入
    if(fd1<0){
        perror("open");
        return 0;
    }
    for(int i=0;i<l1.size();i++){
        if(l1[i]!=username){
            string s=l1[i]+"\n";
            int n1=write(fd1,s.c_str(),s.size());
            if(n1<0&&n1!=s.size()){
                perror("write");
                close(fd1);
                return 0;
            }
        }
    }
    close(fd1);
    return 1;
}