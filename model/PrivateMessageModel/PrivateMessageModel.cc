#include "PrivateMessageModel.h"
#include "../../netlib/base/Logger.h"
#include "../../database/MySQLManager/MySQLManager.h"
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

std::vector<PrivateMessage> PrivateMessageModel::getMessages(const std::string& user1,const std::string& user2){
    vector<PrivateMessage> res;
    MySQLManager mysql;
    if(!mysql.connect()){
    Logger::instance().error( "get private message mysql connect failed");
    return res;
}
    string sql="select fromname,toname,message,createtime "
     "from private_message where (fromname='"+user1+"' and toname='"+user2+"') "
    "or ""(fromname='"+user2+"' and toname='"+user1+"') "
    +   "order by id asc";
    //查->1句柄
    MYSQL* conn=mysql.getConnection();
    if(conn==nullptr) return res;
    //2查
    if(mysql_query(conn,sql.c_str())){
    Logger::instance().error("get private message query failed");
    return res;
}
    //3保存结果
    MYSQL_RES* result=mysql_store_result(conn);
    if(result==nullptr){
    Logger::instance().error("get private message result failed");
    return res;
}
    //行
    MYSQL_ROW row;
    while((row=mysql_fetch_row(result))){
        PrivateMessage msg;
        msg.from=row[0],msg.to=row[1],msg.message=row[2];
        msg.time =row[3]?row[3]:"";
        res.push_back(msg);
    }
    mysql_free_result(result);
    return res;
}
