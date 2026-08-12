#include "FileModel.h"
#include "../../database/MySQLManager/MySQLManager.h"
#include <iostream>
#include "../../netlib/base/Logger.h"
using namespace std;
typedef long long ll;
FileModel::FileModel(){}
FileModel::~FileModel(){}
bool FileModel::saveFileInfo(const string& from,const string& to,const string& filename,ll filesize){
    MySQLManager mysql;
    if(!mysql.connect()){ 
        Logger::instance().error("file mysql connect failed");
        return 0;}
    string sql="insert into file_info(fromname,toname,filename,filesize) values('"+from+"','"+to+"','"+filename+"',"+to_string(filesize)+")";
    if(mysql.execute(sql)){
        Logger::instance().info("save file info success");
        return true;
    }
    Logger::instance().error( "save file info failed");
    return false;
}
bool FileModel::updateFileStatus(const string& fromname,const string& toname,int status){
    MySQLManager manager;
    if(!manager.connect()){
        Logger::instance().error("mysql connect failed");
        return false;
    }
    MYSQL* mysql=manager.getConnection();
    if(mysql==nullptr){
        Logger::instance().error("mysql connection is null");
        return false;
    }

    string sql="update file_info set status="+to_string(status)+" where fromname='"+fromname+"' and toname='" +toname +"'";
    if(mysql_query(mysql,sql.c_str())){
        Logger::instance().error("update file status failed");
        return false;
    }
    return true;
}
string FileModel::getFileName(const string& fromname,const string& toname){
    MySQLManager mysql;
    if(!mysql.connect()){
        Logger::instance().error("mysql connect failed");
        return "";
    }
    string sql="select filename from file_info where fromname='"+fromname+"' and toname='"+toname+"' and status=0";;
    //1获取句柄
    //MYSQL* conn=mysql.getConnection();
    //2查
    MYSQL_RES* res=mysql.query(sql);
    if(res==nullptr){
        Logger::instance().error("query user sql failed");
        return 0;
    }
    MYSQL_ROW row=mysql_fetch_row(res);
    if(row){
        return string(row[0]);
    }
    mysql_free_result(res);
    return "";
}

