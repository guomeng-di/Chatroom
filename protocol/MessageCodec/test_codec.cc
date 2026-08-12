#include <iostream>
#include <string>
#include "MessageCodec.h"
using namespace std;
string encodeBinary(int msgid,const json& js,const std::string& data);
int main()
{
    string data="hello";
    nlohmann::json js;
    auto msg= MessageCodec::encodeBinary(35, js, data);

    int msgid;
    int len;
    MessageCodec::decodeHeader(msg,msgid,len);

    cout<<msgid<<endl;
    cout<<len<<endl;
    return 0;
}