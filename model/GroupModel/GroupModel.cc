 #include "GroupModel.h"
//  #include "../../netlib/base/Logger.h"
 #include "../../database/MySQLManager/MySQLManager.h"
 #include <iostream>
 using namespace std;
 GroupModel::GroupModel(){
 }
 GroupModel::~GroupModel(){
 }
 bool GroupModel::createGroup(const string& groupName,const string& owner,const unordered_set<string>& members){
    MySQLManager mysql;
    if(!mysql.connect()){
        // Logger::instance().error("create group mysql connect failed");
        return false;
    }
    if(!mysql.execute("START TRANSACTION")){
        // Logger::instance().error("create group start transaction failed");
        return false;
    }
    string sql="insert into chat_group(groupname,owner) values('"+groupName+"','"+owner+"')";
    if(!mysql.execute(sql)){
        mysql.execute("ROLLBACK");
        // Logger::instance().error("create group insert chat_group failed");
        return false;
    }
    sql="insert into group_member(groupname,username) values('"+groupName+"','"+owner+"')";
    if(!mysql.execute(sql)){
        mysql.execute("ROLLBACK");
        // Logger::instance().error("create group insert owner member failed");
        return false;
    }
    for(const auto& member:members){
        sql="insert into group_member(groupname,username) values('"+groupName+"','"+member+"')";
        if(!mysql.execute(sql)){
            mysql.execute("ROLLBACK");
            // Logger::instance().error("create group insert member failed:"+member);
            return false;
        }
    }
    if(!mysql.execute("COMMIT")){
        mysql.execute("ROLLBACK");
        // Logger::instance().error("create group commit failed");
        return false;
    }
    // Logger::instance().info("create group success");
    return true;
}
bool GroupModel::addMember(const string& groupName,const string& username){
    MySQLManager mysql;

    if(!mysql.connect()){
        return false;
    }

    string checkSql="select groupname from chat_group where groupname='"+groupName+"'";

    if(!mysql.query(checkSql)){
        return false;
    }

    string sql="insert into group_member(groupname,username) values('"+groupName+"','"+username+"')";

    return mysql.execute(sql);
}

 bool GroupModel::leaveGroup(const string& groupName,const string& username){
     MySQLManager mysql;
     if(!mysql.connect()){
    //  Logger::instance().error("leave group mysql connect failed");
     return false;
 }
     string sql="delete from group_member where groupname='"+groupName+"' and username='"+username+"'";
     if(mysql.execute(sql)){
    //  Logger::instance().info("leave group success");
     return true;
 }
//  Logger::instance().error("leave group failed");
 return false;
 }
 unordered_set<string> GroupModel::getMembers(const string& groupName){
     unordered_set<string> res;
     MySQLManager mysql;
     if(!mysql.connect()){
    //  Logger::instance().error("get group members mysql connect failed");
     return res;
 }
     string sql="select username from group_member where groupname='"+groupName+"'";
     MYSQL* conn=mysql.getConnection();
     if(mysql_query(conn,sql.c_str())){
    //  Logger::instance().error("get group members query failed");
     return res;
 }
     MYSQL_RES* result=mysql_store_result(conn);
     if(result==nullptr) return res;
     MYSQL_ROW row;
     while((row=mysql_fetch_row(result))) res.insert(row[0]);
     mysql_free_result(result);
     return res;
 }
 unordered_set<string> GroupModel::getGroups(const std::string& username){
     unordered_set<string> res;
     MySQLManager mysql;
     if(!mysql.connect()){
        //  Logger::instance().error("get user groups mysql connect failed");
         return res;
     }
     string sql="select groupname from group_member where username='"+username+"'";
     MYSQL* conn=mysql.getConnection();
     if(mysql_query(conn,sql.c_str())){
        //  Logger::instance().error("get user groups query failed");
         return res;
 }
     MYSQL_RES* result=mysql_store_result(conn);
     if(result==nullptr) return res;
     MYSQL_ROW row;
     while((row=mysql_fetch_row(result))) res.insert(row[0]);
     mysql_free_result(result);
     return res;
 }
 bool GroupModel::groupExist(const string& groupName){
     MySQLManager mysql;
     if(!mysql.connect()) return 0;
     string sql="select groupname from chat_group where groupname='"+groupName+"'";
     MYSQL* conn=mysql.getConnection();
     if(mysql_query(conn,sql.c_str())){
    //  Logger::instance().error("check group exist query failed");
     return false;
 }
     MYSQL_RES* result=mysql_store_result(conn);
     if(result==nullptr) return 0;
     int rows=mysql_num_rows(result);
     mysql_free_result(result);
     return rows>0;
 }
 bool GroupModel::isMember(const string& groupName, const string& username){
     MySQLManager mysql;
     if(!mysql.connect()) return 0;
     string sql ="select username from group_member where groupname='"+groupName+"' and username='"+username+"'";
     MYSQL* conn=mysql.getConnection();
     if(mysql_query(conn,sql.c_str())){
    //  Logger::instance().error("check group exist query failed");
     return false;
 }
     MYSQL_RES* result=mysql_store_result(conn);
     if(result==nullptr) return 0;
     int rows=mysql_num_rows(result);
     mysql_free_result(result);
     return rows>0;
 }
 bool GroupModel::isOwner(const string& groupname,const string& username){
     MySQLManager mysql;
     if(!mysql.connect())return false;
     string sql="select * from chat_group where groupname='"+groupname+"' and owner='"+username+"'";
     MYSQL_RES* result=mysql.query(sql);
     if(result){
         MYSQL_ROW row=mysql_fetch_row(result);
         mysql_free_result(result);
         if(row)return true;
     }
     return false;
 }
 bool GroupModel::isAdmin(const string& groupname,const string& username){
     MySQLManager mysql;
     if(!mysql.connect())return false;
     string sql="select * from group_admin where groupname='"+groupname+"' and username='"+username+"'";
     MYSQL_RES* result=mysql.query(sql);
     if(result){
         MYSQL_ROW row=mysql_fetch_row(result);
         mysql_free_result(result);
         if(row)return true;
     }
     return false;
 }
 bool GroupModel::removeMember(const string& groupname,const string& username){
     MySQLManager mysql;
     if(!mysql.connect())return false;
     string sql="delete from group_member where groupname='"+groupname+"' and username='"+username+"'";
     return mysql.execute(sql);
 }
 bool GroupModel::deleteGroup(const string& groupname){
     MySQLManager mysql;
     if(!mysql.connect())return false;
     cout<<"delete group name=["<<groupname<<"]"<<endl;
     string sql1="delete from group_admin where groupname='"+groupname+"'";
     string sql2="delete from group_member where groupname='"+groupname+"'";
     string sql3="delete from chat_group where groupname='"+groupname+"'";
     cout<<sql3<<endl;
     bool r1=mysql.execute(sql1);
     bool r2=mysql.execute(sql2);
     bool r3=mysql.execute(sql3);
     cout<<"delete result:"<<r1<<" "<<r2<<" "<<r3<<endl;
     return r1&&r2&&r3;
 }
 bool GroupModel::addAdmin(const string& groupname,const string& username){
     MySQLManager mysql;
     if(!mysql.connect())return false;
     string sql="insert into group_admin (groupname,username) values('"+groupname+"','"+username+"')";
     return mysql.execute(sql);
 }
 bool GroupModel::removeAdmin(const string& groupname,const string& username){
     MySQLManager mysql;
     if(!mysql.connect())return false;
     string sql="delete from group_admin where groupname='"+groupname+"' and username='"+username+"'";
     return mysql.execute(sql);
 }
 string GroupModel::getOwner(const string& groupname){
     MySQLManager mysql;
     if(!mysql.connect()) return "";
     string sql="select owner from chat_group where groupname='"+groupname+"'";
     MYSQL_RES* result=mysql.query(sql);
     if(result){
         MYSQL_ROW row=mysql_fetch_row(result);
         mysql_free_result(result);
         if(row) return row[0];
     }
     return "";
 }
 unordered_set<string> GroupModel::getAdmins(const string& groupname){
     unordered_set<string> res;
     MySQLManager mysql;
     if(!mysql.connect())return res;
     string sql="select username from group_admin where groupname='"+groupname+"'";
     MYSQL_RES* result=mysql.query(sql);
     if(result) {
         MYSQL_ROW row;
         while((row=mysql_fetch_row(result))) res.insert(row[0]);
         mysql_free_result(result);
     }
     return res;
 }
 bool GroupModel::removeAllGroups(const string& username){
     MySQLManager mysql;
     if(!mysql.connect()){
        //  Logger::instance().error("delete group member mysql connect failed");
         return false;
     }
     string sql ="delete from group_member where username='"+username+"'";
     if(mysql.execute(sql)){
        //  Logger::instance().info(username+ " delete group member success");
         return true;
     }
    //  Logger::instance().error(username+" delete group member failed");
     return false;
 }
 bool GroupModel::removeAdmin_(const string& username){
     MySQLManager mysql;
     if(!mysql.connect()) return false;
     string sql ="delete from group_admin where username='"+username+"'";
     if(mysql.execute(sql)){
        //  Logger::instance().info("remove group admin success");
         return true;
     }
    //  Logger::instance().error(username+" delete group admin failed");
     return false;
 }
 bool GroupModel::removeOwnerGroups(const string& username){
     MySQLManager mysql;
     if(!mysql.connect())return false;
     MYSQL* conn=mysql.getConnection();
     string selectSql="select groupname from chat_group where owner='"+username+"'";
     if(mysql_query(conn,selectSql.c_str()))return false;
     MYSQL_RES* result=mysql_store_result(conn);
     if(result==nullptr) return false;
     MYSQL_ROW row;
     while((row=mysql_fetch_row(result))){
         string groupname=row[0];
         mysql.execute("delete from group_member where groupname='"+groupname+"'");
         mysql.execute("delete from group_admin where groupname='"+groupname+"'");
         mysql.execute("delete from chat_group where groupname='"+groupname+"'");
     }
     mysql_free_result(result);
     return true;
 }
 bool GroupModel::deleteAdmin(const string& groupname,const string& username){
     MySQLManager mysql;
     if(!mysql.connect()) return false;
     string sql ="delete from group_admin where groupname='"+groupname+"' and username="+username+"'";
     if(mysql.execute(sql)){
         return true;
     }
     return false;
 }