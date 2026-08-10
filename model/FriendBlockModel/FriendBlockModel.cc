#include "FriendBlockModel.h"
#include "../../database/MySQLManager/MySQLManager.h"
#include "../../netlib/base/Logger.h"

using namespace std;
bool FriendBlockModel::addBlock(const string& username,const string& blockname){
    MySQLManager mysql;
    if(!mysql.connect()){
        Logger::instance().error("add block mysql connect failed");
        return 0;
    }
    string sql="insert ignore into friend_block(username,blockname) values('"+username+"','"+blockname+"')";
    if(mysql.execute(sql)){
        Logger::instance().info("add block success");
        return 1;
    }
    Logger::instance().error("add block failed");
    return 0;
}
bool FriendBlockModel::removeBlock(const string& username,const string& blockname){
    MySQLManager mysql;
    if(!mysql.connect()){
        Logger::instance().error("remove block mysql connect failed");
        return 0;
    }
    string sql="delete from friend_block where username='"+username+"' and blockname='"+blockname+"'";
    if(mysql.execute(sql)){
        if(mysql_affected_rows(mysql.getConnection())>0){
            Logger::instance().info("remove block success");
            return true;
        }
        Logger::instance().error("unblock nobody");
        return 0;
    }
    Logger::instance().error("remove block failed");
    return 0;
}
bool FriendBlockModel::isBlocked(const string& username,const string& blockname){
    MySQLManager mysql;
    if(!mysql.connect()){
        Logger::instance().error("check block mysql connect failed");
        return 0;
    }
    string sql="select blockname from friend_block where username='"+username+"' and blockname='"+blockname+"'";
    //查询需要获得mysql句柄
    MYSQL* conn=mysql.getConnection();
    //查询失败
    if(mysql_query(conn,sql.c_str())){
        Logger::instance().error("check blockname query failed");
        return 0;
    }
    //保存查询结果到内存
    MYSQL_RES* result=mysql_store_result(conn);
    if(result==nullptr){
    Logger::instance().error( "check blockname result empty");
    return false;
    }

    //获取行数
    int rows=mysql_num_rows(result);
    mysql_free_result(result);
    return rows>0;
}