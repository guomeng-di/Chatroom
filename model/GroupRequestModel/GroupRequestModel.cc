#include "GroupRequestModel.h"
#include "../../database/MySQLManager/MySQLManager.h"
#include "../../netlib/base/Logger.h"
using namespace std;

GroupRequestModel::GroupRequestModel(){}
GroupRequestModel::~GroupRequestModel(){}

bool GroupRequestModel::addRequest(const string& groupname,const string& username){
    MySQLManager mysql;
    if(!mysql.connect())return false;
    string sql =
    "insert into group_request(groupname,username)"
    " values('"+groupname+"','"+username+"')";
    if(mysql.execute(sql)){
        Logger::instance().info(
            "add group request success"
        );
        return true;
    }
    Logger::instance().error(
        "add group request failed"
    );
    return false;
}
vector<GroupRequest> GroupRequestModel::getRequests(const string& groupname){
    vector<GroupRequest> res;
    MySQLManager mysql;
    if(!mysql.connect())return res;
    string sql= "select username,create_time ""from group_request ""where groupname='"+groupname+"'";
    MYSQL_RES* result=mysql.query(sql);
    if(result==nullptr)return res;
    MYSQL_ROW row;
    while((row=mysql_fetch_row(result))){
        GroupRequest request;
        request.groupname=groupname;
        request.username=row[0];
        request.time=row[1]?row[1]:"";
        res.push_back(request);
    }
    mysql_free_result(result);
    return res;
}
bool GroupRequestModel::deleteRequest(const string& groupname,const string& username){
    MySQLManager mysql;
    if(!mysql.connect()) return 0;
    string sql="delete from group_request where groupname='"+groupname+"' and username='"+username+"'";
    return mysql.execute(sql);
}
bool GroupRequestModel::removeGroupRequest(const string& username){
    MySQLManager mysql;
    if(!mysql.connect()) return 0;
    string sql ="delete from group_request where username='" + username + "'";
    return mysql.execute(sql);
}
