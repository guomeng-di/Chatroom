#include "FriendModel.h"
#include "../../database/MySQLManager/MySQLManager.h"
#include <iostream>
// #include "../../netlib/base/Logger.h"
using namespace std;
//unordered_map<std::string,std::unordered_set<string>> FriendModel::friends_;
FriendModel::FriendModel(){

}
FriendModel::~FriendModel(){

}
bool FriendModel::addFriend(const string& username,const string& friendname){
    if(username==friendname||isFriend(username,friendname)) return 0;
    MySQLManager mysql;
    if(!mysql.connect()){ 
        // Logger::instance().error("add friend mysql connect failed");
        return 0;}
    string sql1 ="insert into friend(username,friendname) values('"+username+"','"+friendname+"')";
    string sql2 ="insert into friend(username,friendname) values('"+friendname+"','"+username+"')";
    if(mysql.execute(sql1)&&mysql.execute(sql2)){
        // Logger::instance().info("insert friend relation success");
        return 1;
    }
        // Logger::instance().error("insert friend relation failed");
        return 0;
}
bool FriendModel::isFriend(const string& username,const string& friendname){
    MySQLManager mysql;
    if(!mysql.connect()){ 
        // Logger::instance().error("check friend mysql connect failed");
        return 0;}
        string sql =
    "select * from friend where "
    "(username='"+username+"' and friendname='"+friendname+"') "
    "or "
    "(username='"+friendname+"' and friendname='"+username+"')";
    //string sql="select friendname from friend where username='"+username+"' and friendname='"+friendname+"'";
    //查询需要获得mysql句柄
    MYSQL* conn=mysql.getConnection();
    //查询失败
    if(mysql_query(conn,sql.c_str())){ 
        // Logger::instance().error("check friend query failed");
        return 0;
    }
    //保存查询结果到内存
    MYSQL_RES* result=mysql_store_result(conn);
    if(result==nullptr){
    // Logger::instance().error( "check friend result empty");
    return false;
    }

    //获取行数
    int rows=mysql_num_rows(result);
    mysql_free_result(result);
    return rows>0;
}
unordered_set<string> FriendModel::getFriends(const string& username){
    // auto it=friends_.find(user);
    // if(it==friends_.end()) return unordered_set<string>();
    // return it->second;
    unordered_set<string> friends;
    MySQLManager mysql;
    if(!mysql.connect()) return friends;
    string sql="select friendname from friend where username='"+username+"'";
    MYSQL* conn=mysql.getConnection();
    if(mysql_query(conn,sql.c_str())){
//         Logger::instance().error(
//     "get friend list query failed"
// );
        //cout<<"query error:"<<mysql_error(conn)<<endl;
        return friends;
    }
    MYSQL_RES* result=mysql_store_result(conn);
    if(result==nullptr){
    // Logger::instance().error(
    //     "get friend list result failed"
    // );
    return friends;
}
    MYSQL_ROW row;
    while((row=mysql_fetch_row(result)))
        friends.insert(row[0]);
    
    mysql_free_result(result);
    return friends;

}
bool FriendModel::removeFriend(const std::string& username,const std::string& friendname){
    MySQLManager mysql;
    if(!mysql.connect()){
    // Logger::instance().error( "delete friend mysql connect failed");
    return false;
}
    //删除好友关系
    string sql1 =
    "delete from friend where "
    "(username='"+username+"' and friendname='"+friendname+"') "
    "or "
    "(username='"+friendname+"' and friendname='"+username+"')";

    if(!mysql.execute(sql1)){
        // Logger::instance().error(
        //     "delete friend relation failed"
        // );
        return false;
    }

    //删除屏蔽关系
    string sql2 =
    "delete from friend_block where "
    "(username='"+username+"' and blockname='"+friendname+"') "
    "or "
    "(username='"+friendname+"' and blockname='"+username+"')";

if(!mysql.execute(sql2)){
        // Logger::instance().error(
        //     "delete friend block failed"
        // );
        //这里不要return false
        //好友已经删除成功
    }
    return 1;
}
bool FriendModel::removeAllFriends(const string& username){
    MySQLManager mysql;
    if(!mysql.connect()){
        // Logger::instance().error("delete all friends mysql connect failed");
        return false;
    }
    string sql="delete from friend where username='"+username+"' or friendname='"+username+"'";
    if(mysql.execute(sql)){

        // Logger::instance().info(username+" delete all friends success");
        return true;
    }
    // Logger::instance().error(username+" delete all friends failed");
    return false;
}