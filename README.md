# Chatroom编译方式:

## 配置:
1. nlohmann/json
ubuntu:  
```
sudo apt install nlohmann-json3-dev
```
2. MySql
```
sudo apt install libmysqlclient-dev
```
3. Redis
```
sudo apt install libhiredis-dev
```
4. glog
```
sudo apt install libgoogle-glog-dev
```
5. gtest
```
sudo apt install libgtest-dev
```

## 编译:

### 服务器:
```
cd chatroom_prac/chatroom/chatroom/chatroom  
mkdir build  
cd build  
cmake ..  
make -j16  
./server 
``` 

### 客户端:
```
cd chatroom_prac/chatroom/chatroom/chatroom/build  
./client  
```