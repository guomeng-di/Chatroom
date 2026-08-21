#ifndef ACCOUNT_MENU_H
#define ACCOUNT_MENU_H
#include <string>
class AccountMenu{
public:
    static void run(int fd,const std::string& username);
private:
    static bool sendVerifyCode(int fd,const std::string& email);
};
#endif