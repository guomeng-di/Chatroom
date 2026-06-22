# 为什么需要json
不用每次都一点一点拆解字符串了,json自动实现  
# 聊天室中json放在哪里
客户端:  
用户输入->构造json->send()  
服务器:  
recv()->解析json->处理业务  
# json的定义和对象创建
1定义别名 using json=nlohmann::json;  
2对象创建举例:  
```c++
json js;
js["name"]="tom";
js["age"]=18;
//转成字符串 dump
string s=js.dump();
cout<<s<<endl;
//打印:
{"age":18,"name":"tom"}
```
3dump实现json转字符串  
4parse实现字符串转json   
eg:  
收到:
```c++
string s=
R"(
{
    "name":"tom",
    "age":18
}
)";
//注意:R表示保留原字符串格式内容  
```
解析:
```c++
json js=json::parse(s);
//取数据:
cout<<js["name"]<<endl;
cout<<js["age"]<<endl;
//结果:
tom
18
```