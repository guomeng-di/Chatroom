#include "FileModel.h"
#include "../../database/MySQLManager/MySQLManager.h"
#include <iostream>
#include <cstdlib>
// #include "../../netlib/base/Logger.h"
using namespace std;
typedef long long ll;
FileModel::FileModel(){}
FileModel::~FileModel(){}
bool FileModel::saveFileInfo(const string& from,const string& to,const string& groupname,const string& targetType,const string& filename,ll filesize,const string& filepath){
    MySQLManager mysql;
    if(!mysql.connect()){
        cout<<"file mysql connect failed"<<endl;
        return false;
    }
    MYSQL* conn=mysql.getConnection();
    if(conn==nullptr){
        cout<<"mysql connection is null"<<endl;
        return false;
    }
    auto escape=[conn](const string& input){
        string output;
        output.resize(input.size()*2+1);
        unsigned long len=mysql_real_escape_string(conn,&output[0],input.c_str(),input.size());
        output.resize(len);
        return output;
    };
    string safeFrom=escape(from);
    string safeTo=escape(to);
    string safeGroupname=escape(groupname);
    string safeTargetType=escape(targetType);
    string safeFilename=escape(filename);
    string safeFilepath=escape(filepath);
    string sql="insert into file_info (fromname,toname,groupname,targetType,filename,filepath,filesize) values ('"+safeFrom+"','"+safeTo+"','"+safeGroupname+"','"+safeTargetType+"','"+safeFilename+"','"+safeFilepath+"',"+to_string(filesize)+")";
    cout<<"save file info SQL:"<<sql<<endl;
    if(mysql.execute(sql)){
        cout<<"save file info success"<<endl;
        return true;
    }
    cout<<"save file info failed"<<endl;
    return false;
}

bool FileModel::updateFileStatus(const string& fromname,const string& toname,const string& filename,int status){
    MySQLManager manager;
    if(!manager.connect()){
        // Logger::instance().error("mysql connect failed");
        return false;
    }
    MYSQL* mysql=manager.getConnection();
    if(mysql==nullptr){
        // Logger::instance().error("mysql connection is null");
        return false;
    }

    string sql="update file_info set status="+to_string(status)+" where fromname='"+fromname+"' and toname='" +toname  +"' and filename='"+filename+"'";
    if(mysql_query(mysql,sql.c_str())){
        // Logger::instance().error("update file status failed");
        return false;
    }
    return true;
}
string FileModel::getFileName(const string& fromname,const string& toname){
    MySQLManager mysql;
    if(!mysql.connect()){
        // Logger::instance().error("mysql connect failed");
        return "";
    }
    string sql="select filename from file_info where fromname='"+fromname+"' and toname='"+toname+"' and status=0";;
    //1获取句柄
    //MYSQL* conn=mysql.getConnection();
    //2查
    MYSQL_RES* res=mysql.query(sql);
    if(res==nullptr){
        // Logger::instance().error("query user sql failed");
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
        // Logger::instance().error("mysql connect failed");
        return "";
    }
    string sql ="select id from file_info where ""fromname='"+fromname+ "' and toname='"+toname+"' and filename='"+filename+"' and status in (0,1)";
    cout<<"check sql:"<<sql<<endl;
    MYSQL_RES* res=mysql.query(sql);
    if(res==nullptr){
        // Logger::instance().error("query user sql failed");
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
        // Logger::instance().error("mysql connect failed");
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
        // Logger::instance().error("mysql connect failed");
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
        // Logger::instance().error("get file size mysql connect failed");
        return -1;
    }

    string sql =
        "select filesize "
        "from file_info "
        "where id=" + to_string(fileid);

    cout << "getFileSize SQL:" << sql << endl;

    MYSQL_RES* res = mysql.query(sql);

    if(!res){
        // Logger::instance().error("get file size query failed");
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(res);

    if(!row){
        mysql_free_result(res);
        // Logger::instance().error("file not found");
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



    MYSQL* conn=mysql.getConnection();
    if(conn==nullptr)
        return false;

    string safeReceiver;
    safeReceiver.resize(receiver.size()*2+1);
    unsigned long len=mysql_real_escape_string(
        conn,
        &safeReceiver[0],
        receiver.c_str(),
        receiver.size()
    );
    safeReceiver.resize(len);

    // 重复接受同一个文件时保持原有接收进度，不重复插入记录。
    string sql =
    "insert into file_receiver(fileid,receiver,status,received_size) "
    "select " + to_string(fileid) + ",'" + safeReceiver + "',0,0 "
    "where not exists (select 1 from file_receiver where fileid="
    + to_string(fileid) + " and receiver='" + safeReceiver + "')";



    return mysql.execute(sql);
}
bool FileModel::updateFileReceiver(int fileid,const string& receiver,int status){
    MySQLManager mysql;
    if(!mysql.connect()){
        // Logger::instance().error("mysql connect failed");
        return false;
    }
    string sql ="update file_receiver set status="+to_string(status)+" where fileid="+to_string(fileid)+" and receiver='"+receiver+"'";
    if(mysql.execute(sql)) return true;
    // Logger::instance().error("update file receiver failed");
    return false;
}
bool FileModel::updateReceivedSize(int fileid,const string& receiver,long long size){
    MySQLManager mysql;
    if(!mysql.connect()){
        // Logger::instance().error("mysql connect failed");
        return false;
    }

    string sql ="update file_receiver set received_size=greatest(received_size,"+to_string(size)+") where fileid="+to_string(fileid)+" and receiver='"+receiver+"'";

    return mysql.execute(sql);
}
long long FileModel::getReceivedSize(int fileid,const string& receiver){
    MySQLManager mysql;


    if(!mysql.connect())
    {
        // Logger::instance()
        // .error(
        // "mysql connect failed"
        // );

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
        // Logger::instance()
        // .error(
        // "query received size failed"
        // );

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
        // Logger::instance().error("mysql connect failed");
        return false;
    }
    string sql ="select count(*) from file_receiver ""where fileid="+to_string(fileid)+" and status!=2";
    MYSQL_RES* res=mysql.query(sql);
    if(res==nullptr){
        // Logger::instance().error( "check receiver finish failed");
        return false;
    }
    MYSQL_ROW row=mysql_fetch_row(res);
    int unfinished=atoi(row[0]);
    mysql_free_result(res);
    if(unfinished==0)return true;
    return false;
}
vector<string> FileModel::getUnfinishedFiles(const string& username)
{
    vector<string> files;

    MySQLManager mysql;

    if(!mysql.connect()){
        // Logger::instance().error("mysql connect failed");
        return files;
    }

    string sql =
        "select id,fromname,filename,filesize "
        "from file_info "
        "where toname='"+username+"' "
        "and targetType='user' "
        "and status=1";

    cout << "========== GET UNFINISHED FILES ==========" << endl;
    cout << "username=" << username << endl;
    cout << "sql=" << sql << endl;

    MYSQL_RES* res = mysql.query(sql);

    if(res == nullptr){
        // Logger::instance().error("query unfinished files failed");
        cout << "query unfinished files failed" << endl;
        cout << "==========================================" << endl;
        return files;
    }

    MYSQL_ROW row;

    while((row = mysql_fetch_row(res)))
    {
        json js;

        js["fileid"] = atoi(row[0]);
        js["fromname"] = row[1];
        js["filename"] = row[2];
        js["filesize"] = atoll(row[3]);

        cout << "unfinished file:"
             << js.dump()
             << endl;

        files.push_back(js.dump());
    }

    mysql_free_result(res);

    cout << "unfinished file count="
         << files.size()
         << endl;

    cout << "==========================================" << endl;

    return files;
}





int FileModel::getUnfinishedFileId(
    const string& fromname,
    const string& toname,
    const string& filename)
{
    MySQLManager mysql;

    if(!mysql.connect()){
        // Logger::instance().error("mysql connect failed");
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
        // Logger::instance().error(
        //     "query unfinished file id failed"
        // );
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

bool FileModel::getUnfinishedFileInfo(
    const string& receiver,
    const string& filename,
    string& fromname,
    int& fileid,
    long long& filesize)
{
    MySQLManager mysql;
    if(!mysql.connect()){
        // Logger::instance().error("mysql connect failed");
        return false;
    }

    MYSQL* conn = mysql.getConnection();
    if(conn == nullptr){
        // Logger::instance().error("mysql connection is null");
        return false;
    }

    auto escape = [conn](const string& input){
        string output;
        output.resize(input.size() * 2 + 1);
        unsigned long len = mysql_real_escape_string(
            conn,
            &output[0],
            input.c_str(),
            input.size()
        );
        output.resize(len);
        return output;
    };

    string safeReceiver = escape(receiver);
    string safeFilename = escape(filename);

    string sql =
        "select id, fromname, filesize "
        "from file_info "
        "where toname='"+safeReceiver+"' "
        "and filename='"+safeFilename+"' "
        "and targetType='user' "
        "and status=1 "
        "order by id desc limit 1";

    MYSQL_RES* res = mysql.query(sql);
    if(res == nullptr){
        // Logger::instance().error("query unfinished file info failed");
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    if(row == nullptr){
        mysql_free_result(res);
        return false;
    }

    fileid = atoi(row[0]);
    fromname = row[1];
    filesize = atoll(row[2]);

    mysql_free_result(res);
    return true;
}
string FileModel::getFilePath(int fileid){
    MySQLManager mysql;
    if(!mysql.connect()){
        cout<<"get file path mysql connect failed"<<endl;
        return "";
    }
    string sql="select filepath from file_info where id="+to_string(fileid);
    cout<<"getFilePath SQL:"<<sql<<endl;
    MYSQL_RES* res=mysql.query(sql);
    if(res==nullptr){
        cout<<"get file path query failed"<<endl;
        return "";
    }
    MYSQL_ROW row=mysql_fetch_row(res);
    if(row==nullptr){
        mysql_free_result(res);
        cout<<"file path not found, fileid="<<fileid<<endl;
        return "";
    }
    string filepath=row[0]?row[0]:"";
    mysql_free_result(res);
    cout<<"fileid="<<fileid<<" filepath="<<filepath<<endl;
    return filepath;
}
bool FileModel::updateFilePath(int fileid,const string& filepath){
    MySQLManager mysql;
    if(!mysql.connect()){
        cout<<"update file path mysql connect failed"<<endl;
        return false;
    }
    MYSQL* conn=mysql.getConnection();
    if(conn==nullptr){
        cout<<"mysql connection is null"<<endl;
        return false;
    }
    string safePath;
    safePath.resize(filepath.size()*2+1);
    unsigned long len=mysql_real_escape_string(conn,&safePath[0],filepath.c_str(),filepath.size());
    safePath.resize(len);
    string sql="update file_info set filepath='"+safePath+"' where id="+to_string(fileid);
    cout<<"updateFilePath SQL:"<<sql<<endl;
    if(mysql.execute(sql)){
        cout<<"update file path success"<<endl;
        return true;
    }
    cout<<"update file path failed"<<endl;
    return false;
}
