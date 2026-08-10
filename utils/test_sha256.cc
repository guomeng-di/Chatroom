#include "SHA256/SHA256.h"
#include <iostream>
using namespace std;

int main()
{
    string pwd = "123456";
    string res = HashSHA256::encode(pwd);
    cout << "哈希结果：" << res << endl;
    cout << "长度：" << res.size() << endl;
    return 0;
}