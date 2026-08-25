#include "MySQLManager.h"
#include "../../netlib/base/Logger/Logger.h"
#include <iostream>
using namespace std;
MySQLManager::MySQLManager(){
    mysql_=NULL;
}
MySQLManager::~MySQLManager(){
    if(mysql_){
        mysql_close(mysql_);
    }
}
bool MySQLManager::connect(){
    mysql_=mysql_init(nullptr);//初始化失败返回NULL
    if(mysql_==nullptr){
        LOG_ERROR<<"MySQL初始化失败";
        return false;
    }
    MYSQL* ret=mysql_real_connect(
        mysql_,
        "127.0.0.1",//ip
        "root",//用户名
        "0527985",
        "chatroom",//数据库名
        3306,
        nullptr,
        0
    );
    if(ret==nullptr){
        LOG_ERROR<<mysql_error(mysql_);
        return false;
    }
    //LOG_INFO<<"MySQL连接成功";
    return true;
}
//执行SQL语句
bool MySQLManager::execute(const string& sql){
    if(mysql_==nullptr) return false;
    if(mysql_query(mysql_,sql.c_str())){
        LOG_ERROR<<mysql_error(mysql_);
        return 0;
    }
    return 1;
}
MYSQL* MySQLManager::getConnection(){
    return mysql_;
}
MYSQL_RES* MySQLManager::query(const string& sql){
    if(mysql_==nullptr) return nullptr;
    if(mysql_query(mysql_,sql.c_str())){
        LOG_ERROR<<mysql_error(mysql_);
        return nullptr;
    }
    MYSQL_RES* res=mysql_store_result(mysql_);
    return res;
}