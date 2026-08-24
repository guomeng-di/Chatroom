#include "FriendRequestModel.h"
#include "../../database/MySQLManager/MySQLManager.h"
#include <iostream>
// #include "../../netlib/base/Logger.h"

using namespace std;
//初始化静态成员
//unordered_map<string,unordered_set<string>>FriendRequestModel::requests_;
FriendRequestModel::FriendRequestModel(){
}
FriendRequestModel::~FriendRequestModel(){
}
bool FriendRequestModel::addRequest(const string& from,const string& to){
    MySQLManager mysql;
    if(!mysql.connect())
{
    cout<<"mysql connect failed"<<endl;
    return false;
}
    string check="select * from friend_request ""where fromname='"+from+"' and toname='"+to+"'";
   
    MYSQL_RES* res=mysql.query(check);
    if(res){
        MYSQL_ROW row=mysql_fetch_row(res);
        if(row){
            mysql_free_result(res);
            return false;
        }

        mysql_free_result(res);
    }
    string sql="insert into friend_request (fromname,toname) "
    "values('"+from+"','"+to+"')";

    cout<<"execute sql:"
    <<sql
    <<endl;


    return mysql.execute(sql);
}
vector<FriendRequest> FriendRequestModel::getRequests(const string& username){
    vector<FriendRequest> res;
    MySQLManager mysql;
    if(!mysql.connect()) return res;

    string sql =
    "select fromname, create_time "
    "from friend_request "
    "where toname='" + username + "' "
    "order by create_time asc";
    //string sql ="select fromname,create_time ""from friend_request ""where toname='"+username+"' ""order by create_time asc";
    MYSQL* conn=mysql.getConnection();
    if(conn==nullptr) return res;
    if(mysql_query(conn,sql.c_str())){
        // Logger::instance().error(mysql_error(conn));
        return res;
    }
    MYSQL_RES* result=mysql_store_result(conn);
    if(result==nullptr) return res;
    MYSQL_ROW row;
    while((row=mysql_fetch_row(result))){
        FriendRequest request;
        request.from=row[0];
        request.time=row[1]?row[1]:"";
        res.push_back(request);
    }
    mysql_free_result(result);
    return res;
}
bool FriendRequestModel::removeRequest(const string& from,const string& to){
    MySQLManager mysql;
    if(!mysql.connect()) return false;
    string sql =
    "delete from friend_request "
    "where fromname='"
    +from+
    "' and toname='"
    +to+
    "'";
    if(mysql.execute(sql))
      return mysql_affected_rows(mysql.getConnection())>0;
return false;
}

bool FriendRequestModel::removeAllRequests(const string& username){
    MySQLManager mysql;
    if(!mysql.connect()){
        // Logger::instance().error("delete friend request mysql connect failed");
        return 0;
    }
    string sql="delete from friend_request where fromname='"+username+"' or toname='"+username+"'";
    if(mysql.execute(sql)){
        // Logger::instance().info(username+" delete friend request success");
        return true;
    }
    // Logger::instance().error(username+" delete friend request failed");
    return false;
}