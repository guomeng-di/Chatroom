#include "UserModel.h"
// #include "../../netlib/base/Logger.h"
#include "../../database/MySQLManager/MySQLManager.h"
using namespace std;
//unordered_map<string,string> UserModel::users_;
UserModel::UserModel(){

}
UserModel::~UserModel(){

}
bool UserModel::insertUser(const string& username,const string& password,const std::string& email){
    MySQLManager mysql;
    if(!mysql.connect()){
        // Logger::instance().error("insert user mysql connect failed");
        return 0;
    }
    string sql="insert into user(username,password,email) values('"+username+"','"+password+"','"+email+"')";
    if(mysql.execute(sql)){
    // Logger::instance().info("insert user success");
    return true;
}else{
    // Logger::instance().error( "insert user failed");
    return false;
}
}
bool UserModel::queryUser(const string& username,const string& password){
    MySQLManager mysql;
    if(!mysql.connect()){ 
        // Logger::instance().error("querry user mysql connect failed");
        return 0;}
    //写sql语句
    string sql="select * from user where username='"+username+"' and password='"+password+"'";
    MYSQL_RES* res=mysql.query(sql);
    if(res==nullptr){ 
        // Logger::instance().error("query user sql failed");
        return 0;}
    MYSQL_ROW row=mysql_fetch_row(res);
    if(row){
        mysql_free_result(res);
        return 1;
    } 
    mysql_free_result(res);
    return 0;
}
bool UserModel::deleteUser(const string& username){
    MySQLManager mysql;
    //1连接
    if(!mysql.connect()){
    //    Logger::instance().error("delete user mysql connect failed");
        return false; 
    }
    //2写sql语句
    string sql="delete from user where username='"+username+"'";
    //3删
    if(mysql.execute(sql)){
        // Logger::instance().info(username + " delete user success");
        return true;
    }
    // Logger::instance().error(username + " delete user failed");
    return false;
}
bool UserModel::updatePasswordByEmail(const string& email,const string& passwordHash){
    MySQLManager mysql;
    if(!mysql.connect()){
        // Logger::instance().error("update password mysql connect failed");
        return 0;
    }
    string sql="update user set password='"+passwordHash+"' where email='"+email+"'";
    if(mysql.execute(sql)){
        // Logger::instance().info("update password success");
        return 1;
    }
    // Logger::instance().error("update password failed");
    return 0;
}
string UserModel::queryUsernameByEmail(const string& email){
    MySQLManager mysql;
    if(!mysql.connect()){
        // Logger::instance().error("query username by email by mysql connect failed");
        return "";
    }
    string sql="select username from user where email='"+email+"'";
    MYSQL_RES* res=mysql.query(sql);
    if(res==nullptr){ 
        // Logger::instance().error("query user sql failed");
        return "";
    }
    MYSQL_ROW row=mysql_fetch_row(res);
    if(row){
        mysql_free_result(res);
        // Logger::instance().info("query username by email:"+email);
        return string(row[0]);
    } 
    mysql_free_result(res);
    return "";
}
bool UserModel::queryUserByUsername(const string& username){
    MySQLManager mysql;
    if(!mysql.connect()){
        // Logger::instance().error("query user by username mysql connect failed");
        return false;
    }
    string sql ="select username from user where username='" + username + "'";
    MYSQL_RES* res = mysql.query(sql);
    if(res == nullptr){
        // Logger::instance().error("query user by username sql failed");
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    if(row){
        mysql_free_result(res);
        // Logger::instance().info("user exists: " + username);
        return true;
    }
    mysql_free_result(res);
    // Logger::instance().info("user not exist: " + username);
    return false;
}