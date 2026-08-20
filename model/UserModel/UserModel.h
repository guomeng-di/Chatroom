//model负责：数据怎么保存和查询
//初次登录
#pragma once
#include <string>
#include <unordered_map>
#include <iostream>
class UserModel{
    public:
      UserModel();
      ~UserModel();

      static bool insertUser(const std::string& username,const std::string& password,const std::string& email);//用户插入
      static bool queryUser(const std::string& username,const std::string& password);//用户查询
      static bool queryUserByUsername(const std::string& username);
      //用户删除(注销账号)
      bool deleteUser(const std::string& username);
      //重置密码
      bool updatePasswordByEmail(const std::string& email,const std::string& passwordHash);
      //根据邮箱查查找用户名
      std::string queryUsernameByEmail(const std::string& email);
    
    // private:
    //   static std::unordered_map<std::string,std::string> users_;
};