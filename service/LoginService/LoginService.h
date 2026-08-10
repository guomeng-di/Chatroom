#pragma once
#include <nlohmann/json.hpp>
class TcpConnection;
class OnlineUserManager;
using json=nlohmann::json;

class LoginService{
    public:
      LoginService();
      ~LoginService();

      static json login(const json& js,TcpConnection* conn);//比json js好,这个只查看,不复制
      //static表示:login属于这个类，但是不依赖某个对象.调用时不用new LoginService()
};
extern OnlineUserManager onlineUserManager;