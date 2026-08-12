#pragma once

#include <mysql/mysql.h>
#include <string>

class MySQLManager{
    public:
      MySQLManager();
      ~MySQLManager();

      //连接数据库
      bool connect();
      //执行SQL语句(增删改)
      bool execute(const std::string& sql);
      //执行SQL语句(查)
      MYSQL_RES* query(const std::string& sql);
      //获取mysql句柄
      MYSQL* getConnection();

    private:
      MYSQL* mysql_;
};
// 类比：去饭店吃饭
// MySQL 数据库 = 饭店
// 你写的程序（聊天室 server）= 顾客
// MYSQL 句柄 = 一张餐桌号 / 就餐通行证*
// 完整流程对照
// mysql_init()  
// 你打电话给饭店：帮我预留一张桌子，饭店分配一张餐桌，把桌号交给你。
// mysql_ 保存这个桌号。
// mysql_real_connect()
// 拿着桌号到前台核验身份（账号、密码、数据库名），正式入座，建立连接。
// mysql_query(sql)
// 拿着桌号，服务员根据桌号找到你，接收你的点菜指令（SQL 语句）。
// mysql_close(mysql_)
// 吃完饭，凭桌号办理退房，释放餐桌。