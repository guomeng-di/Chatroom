#include "GroupMessageModel.h"
#include "../../database/MySQLManager/MySQLManager.h"
#include "../../netlib/base/Logger.h"

using namespace std;

GroupMessageModel::GroupMessageModel(){}
GroupMessageModel::~GroupMessageModel(){}
// 保存群消息
bool GroupMessageModel::saveMessage(const string& groupname,const string& from,const string& message){
    MySQLManager mysql;
    if(!mysql.connect()){
        Logger::instance().error("save group message mysql connect failed");
        return false;
    }
    string sql ="insert into group_message(groupname,fromname,message) values('"+ groupname + "','"+ from + "','"+ message + "')";

    if(mysql.execute(sql)){Logger::instance().info("save group message success");
        return true;
    }
    Logger::instance().error("save group message failed");
    return false;
}

// 查询群历史消息
vector<GroupMessage> GroupMessageModel::getMessages(const string& groupname){
    vector<GroupMessage> res;
    MySQLManager mysql;
    if(!mysql.connect()){
        Logger::instance().error("get group message mysql connect failed");
        return res;
    }
    string sql ="select groupname,fromname,message,createtime ""from group_message ""where groupname='" + groupname + "' ""order by id asc";
    MYSQL* conn = mysql.getConnection();
    if(conn==nullptr)return res;
    if(mysql_query(conn, sql.c_str())){
        Logger::instance().error("get group message query failed");
        return res;
    }
    MYSQL_RES* result = mysql_store_result(conn);
    if(result == nullptr){
        Logger::instance().error( "get group message result failed");
        return res;
    }
    MYSQL_ROW row;
    while((row = mysql_fetch_row(result))){
        GroupMessage msg;
        msg.groupname = row[0] ? row[0] : "";
        msg.from      = row[1] ? row[1] : "";
        msg.message   = row[2] ? row[2] : "";
        msg.time      = row[3] ? row[3] : "";
        res.push_back(msg);
    }
    mysql_free_result(result);
    Logger::instance().info("get group history success, count=" + to_string(res.size()));
    return res;
}