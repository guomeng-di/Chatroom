#include "PrivateMessageModel.h"
#include "../../netlib/base/Logger.h"
#include "../../database/MySQLManager/MySQLManager.h"
#include <algorithm>
using namespace std;
PrivateMessageModel::PrivateMessageModel(){

}
PrivateMessageModel::~PrivateMessageModel(){

}
bool PrivateMessageModel::saveMessage(std::string from,std::string to,std::string message){
    MySQLManager mysql;
    if(!mysql.connect()){ 
        Logger::instance().error("save private message mysql connect failed");
        return 0;
    }
    string sql="insert into private_message(fromname,toname,message) values('"+from+"','"+to+"','"+message+"')";
    if(mysql.execute(sql)){
    Logger::instance().info("save private message success");
    return true;
}
Logger::instance().error("save private message failed");
return false;
}

std::vector<PrivateMessage> PrivateMessageModel::getMessages(const std::string& user1,const std::string& user2,long long beforeId){
    vector<PrivateMessage> res;
    MySQLManager mysql;
    //1connect
    if(!mysql.connect()){
     Logger::instance().error("get private message mysql connect failed");
     return res;
    }
    //2sql
    //2.1beforeId==0->最新50条
    //2.2beforeId>0->(beforeId=151->查100-150)
    string sql="select id,fromname,toname,message,createtime from private_message where ((fromname='"+user1+"' and toname='"+user2+"') or (fromname='"+user2+"' and toname='"+user1+"'))";
    if(beforeId>0) sql+=" and id<"+to_string(beforeId);
    sql+=" order by id desc limit 50";
    MYSQL* conn=mysql.getConnection();
    if(conn==nullptr) return res;
    //3查
    if(mysql_query(conn,sql.c_str())){
     Logger::instance().error("get private message query failed");
     return res;
    }
    MYSQL_RES* result=mysql_store_result(conn);
    if(result==nullptr){
     Logger::instance().error("get private message result failed");
     return res;
    }
    MYSQL_ROW row;
    while((row=mysql_fetch_row(result))){
     PrivateMessage msg;
     msg.id=row[0]?stoll(row[0]):0;
     msg.from=row[1]?row[1]:"";
     msg.to=row[2]?row[2]:"";
     msg.message=row[3]?row[3]:"";
     msg.time=row[4]?row[4]:"";
     res.push_back(msg);
    }
    mysql_free_result(result);
    reverse(res.begin(),res.end());
    return res;
}