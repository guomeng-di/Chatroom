#include "FileModel.h"
#include "../../database/MySQLManager/MySQLManager.h"
#include <iostream>
#include <cstdlib>
#include "../../netlib/base/Logger.h"
using namespace std;
typedef long long ll;
FileModel::FileModel(){}
FileModel::~FileModel(){}
bool FileModel::saveFileInfo(const string& from,const string& to,const string& groupname,const string& targetType,const string& filename,ll filesize){
    MySQLManager mysql;
    if(!mysql.connect()){ 
        Logger::instance().error("file mysql connect failed");
        return 0;
    }
    string sql="insert into file_info(""fromname,""toname,""groupname,""targetType,""filename,""filesize"") values('"+from+"','"+to+"','"+groupname+"','"+targetType+"','"+filename+"',"+to_string(filesize)+")";
    
    if(mysql.execute(sql)){
        Logger::instance().info("save file info success");
        return true;
    }
    Logger::instance().error( "save file info failed");
    return false;
}

bool FileModel::updateFileStatus(const string& fromname,const string& toname,const string& filename,int status){
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

    string sql="update file_info set status="+to_string(status)+" where fromname='"+fromname+"' and toname='" +toname  +"' and filename='"+filename+"'";
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
        string filename(row[0]);

        mysql_free_result(res);

        return filename;
    }
    mysql_free_result(res);
    return "";
}
bool FileModel::checkFileRequest(const string& fromname,const string& toname,const string& filename){
    MySQLManager mysql;
    if(!mysql.connect()){
        Logger::instance().error("mysql connect failed");
        return "";
    }
    string sql ="select id from file_info where ""fromname='"+fromname+ "' and toname='"+toname+"' and filename='"+filename+"' and status in (0,1)";
    cout<<"check sql:"<<sql<<endl;
    MYSQL_RES* res=mysql.query(sql);
    if(res==nullptr){
        Logger::instance().error("query user sql failed");
        return 0;
    }
    MYSQL_ROW row=mysql_fetch_row(res);
    if(row){
        mysql_free_result(res);
        return 1;
    }
    mysql_free_result(res);
    return 0;
}
bool FileModel::checkGroupFileRequest(const string& fromname,const string& groupname,const string& filename){
    MySQLManager mysql;
    if(!mysql.connect()){
        Logger::instance().error("mysql connect failed");
        return false;
    }
    string sql=
    "select id from file_info where "
    "fromname='"+fromname+
    "' and groupname='"+groupname+
    "' and filename='"+filename+
    "' and targetType='group' "
    "and status in (0,1)";

    MYSQL_RES* res=mysql.query(sql);
    if(res==nullptr) return false;
    MYSQL_ROW row=mysql_fetch_row(res);
    bool ret=false;
    if(row) ret=true;
    mysql_free_result(res);
    return ret;
}
int FileModel::getFileId(const string& fromname,const string& toname,const string& groupname,const string& targetType,const string& filename){
    MySQLManager mysql;
    if(!mysql.connect()){
        Logger::instance().error("mysql connect failed");
        return -1;
    }

    string sql =
"select id from file_info where "
"fromname='"+fromname+"' and "
"toname='"+toname+"' and "
"groupname='"+groupname+"' and "
"filename='"+filename+"' and "
"targetType='"+targetType+"' "
"order by id desc limit 1";
    //string sql ="select id from file_info where ""fromname='"+fromname+"' and toname='"+toname+"' and groupname='"+groupname+"' and filename='"+filename+"' and targetType='"+targetType+"'";
    //string sql ="select id from file_info where ""fromname='"+fromname+"' and toname='"+toname+"' and filename='"+filename+"'";
    MYSQL_RES* res=mysql.query(sql);
    if(res==nullptr) return -1;
    
    MYSQL_ROW row=mysql_fetch_row(res);
    if(row){
        int id=atoi(row[0]);
        mysql_free_result(res);
        return id;
    }
    mysql_free_result(res);
    return -1;
}
long long FileModel::getFileSize(int fileid)
{
    MySQLManager mysql;

    if(!mysql.connect()){
        Logger::instance().error("get file size mysql connect failed");
        return -1;
    }

    string sql =
        "select filesize "
        "from file_info "
        "where id=" + to_string(fileid);

    cout << "getFileSize SQL:" << sql << endl;

    MYSQL_RES* res = mysql.query(sql);

    if(!res){
        Logger::instance().error("get file size query failed");
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(res);

    if(!row){
        mysql_free_result(res);
        Logger::instance().error("file not found");
        return -1;
    }

    long long filesize = atoll(row[0]);

    mysql_free_result(res);

    cout << "fileid=" << fileid
         << " filesize=" << filesize
         << endl;

    return filesize;
}
bool FileModel::saveFileReceiver(
    int fileid,
    const string& receiver)
{
    MySQLManager mysql;


    if(!mysql.connect())
        return false;



    string sql =
    "insert into file_receiver"
    "(fileid,receiver,status,received_size)"
    " values("
    +to_string(fileid)
    +",'"
    +receiver
    +"',0,0)";



    return mysql.execute(sql);
}
bool FileModel::updateFileReceiver(int fileid,const string& receiver,int status){
    MySQLManager mysql;
    if(!mysql.connect()){
        Logger::instance().error("mysql connect failed");
        return false;
    }
    string sql ="update file_receiver set status="+to_string(status)+" where fileid="+to_string(fileid)+" and receiver='"+receiver+"'";
    if(mysql.execute(sql)) return true;
    Logger::instance().error("update file receiver failed");
    return false;
}
bool FileModel::updateReceivedSize(int fileid,const string& receiver,long long size){
    MySQLManager mysql;
    if(!mysql.connect()){
        Logger::instance().error("mysql connect failed");
        return false;
    }

    string sql ="update file_receiver ""set received_size="+to_string(size)+" where fileid="+to_string(fileid)+" and receiver='"+receiver+"'";

    return mysql.execute(sql);
}
long long FileModel::getReceivedSize(int fileid,const string& receiver){
    MySQLManager mysql;


    if(!mysql.connect())
    {
        Logger::instance()
        .error(
        "mysql connect failed"
        );

        return -1;
    }



    string sql =
    "select received_size "
    "from file_receiver "
    "where fileid="
    +to_string(fileid)+" and receiver='"+receiver+"'";



    MYSQL_RES* res=
        mysql.query(sql);



    if(res==nullptr)
    {
        Logger::instance()
        .error(
        "query received size failed"
        );

        return -1;
    }



    MYSQL_ROW row=
        mysql_fetch_row(res);



    if(row==nullptr)
    {
        mysql_free_result(res);

        return 0;
    }



    long long size=
        atoll(row[0]);



    mysql_free_result(res);


    return size;

}
vector<string> FileModel::getFileReceivers(int fileid){
    vector<string> receivers;
    MySQLManager mysql;
    if(!mysql.connect())return receivers;

    string sql="select receiver from file_receiver where fileid="+to_string(fileid);
    MYSQL_RES* res=mysql.query(sql);
    if(!res)return receivers;
    MYSQL_ROW row;
    while((row=mysql_fetch_row(res)))
        receivers.push_back(row[0]);
    
    mysql_free_result(res);
    return receivers;
}
bool FileModel::checkAllReceiverFinish(int fileid){
    MySQLManager mysql;
    if(!mysql.connect()){
        Logger::instance().error("mysql connect failed");
        return false;
    }
    string sql ="select count(*) from file_receiver ""where fileid="+to_string(fileid)+" and status!=2";
    MYSQL_RES* res=mysql.query(sql);
    if(res==nullptr){
        Logger::instance().error( "check receiver finish failed");
        return false;
    }
    MYSQL_ROW row=mysql_fetch_row(res);
    int unfinished=atoi(row[0]);
    mysql_free_result(res);
    if(unfinished==0)return true;
    return false;
}
vector<string> FileModel::getUnfinishedFiles(const string& username){
    vector<string> files;
    MySQLManager mysql;
    if(!mysql.connect()){
        Logger::instance().error("mysql connect failed");
        return files;
    }
    string sql =
"select fromname,filename,filesize "
"from file_info "
"where toname='"+username+"' "
"and status=1";
    //string sql ="select fromname,filename,filesize ""from file_info ""where toname='"+ username +"' and status!=2";
    MYSQL_RES* res =mysql.query(sql);
    if(res==nullptr){
        Logger::instance().error("query unfinished files failed" );
        return files;
    }
    MYSQL_ROW row;
    while((row=mysql_fetch_row(res))){
        json js;
        js["fromname"] = row[0];
        js["filename"] = row[1];
        js["filesize"] = atoll(row[2]);

        files.push_back(js.dump());
    }
    mysql_free_result(res);
    return files;
}





int FileModel::getUnfinishedFileId(
    const string& fromname,
    const string& toname,
    const string& filename)
{
    MySQLManager mysql;

    if(!mysql.connect()){
        Logger::instance().error("mysql connect failed");
        return -1;
    }

    string sql =
        "select id from file_info "
        "where fromname='"+fromname+"' "
        "and toname='"+toname+"' "
        "and filename='"+filename+"' "
        "and targetType='user' "
        "and status=1 "
        "order by id desc limit 1";

    cout << "get unfinished file id SQL:" << sql << endl;

    MYSQL_RES* res = mysql.query(sql);

    if(res == nullptr){
        Logger::instance().error(
            "query unfinished file id failed"
        );
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(res);

    if(row){
        int fileid = atoi(row[0]);

        mysql_free_result(res);

        cout << "unfinished fileid=" << fileid << endl;

        return fileid;
    }

    mysql_free_result(res);

    return -1;
}


