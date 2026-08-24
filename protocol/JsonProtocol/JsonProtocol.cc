#include "JsonProtocol.h"
#include <stdexcept>
#include <iostream>
#include <exception>
#include <string>  
#include "../../netlib/base/Logger/Logger.h"
using namespace std;
std::string JsonProtocol::encode(const json& js){
    return js.dump();
}
json JsonProtocol::decode(const std::string& msg){//长度+内容
    string jsonStr(msg.data()+4,msg.size()-4);
    return json::parse(jsonStr);
}
// dump()序列化:通过递归,遍历,把json树拼成字符串 
// eg:json js;
// js["msgid"]=FILE_DATA_MSG;
// string str = js.dump();
// dump 把内存里的js树拼成 {"msgid":1001,"filename":"xxx"} 这样的字符串

// parse()反序列化则相反,分词 token + 递归建节点树

// 往后获取filename,username等时先反序列化得到树  
// 因为序列化后变成一长串，无法寻找，反序列化后是树，好寻找
// 在内存建成树形结构(内部是哈希表 + 对象节点),key和value直接建立映射