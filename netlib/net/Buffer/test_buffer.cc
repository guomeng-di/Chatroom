#include "Buffer.h"
#include <iostream>
#include <arpa/inet.h>
using namespace std;

// 构造数据包：4字节网络序长度头 + 消息体
string makePkg(const string &body)
{
    string pkg;
    uint32_t len = htonl(body.size());
    pkg.append((char*)&len, 4);
    pkg += body;
    return pkg;
}

int main()
{
    cout << "====场景1：一次性收到完整包====" << endl;
    Buffer buf1;
    string full = makePkg("hello");
    buf1.append(full.data(), full.size());

    if(buf1.hasMessage())
    {
        string res = buf1.retrieveMessage();
        cout << "hasMessage=true, msg=" << res << endl;
    }
    else
    {
        cout << "hasMessage=false" << endl;
    }

    cout << "\n====场景2：分包接收====" << endl;
    Buffer buf2;
    string pkg = makePkg("hello");
    // 先传前7个字节
    buf2.append(pkg.data(), 7);
    if(!buf2.hasMessage())
    {
        cout << "第一次接收半包：hasMessage=false" << endl;
    }
    // 追加剩下数据
    buf2.append(pkg.data()+7, pkg.size()-7);
    if(buf2.hasMessage())
    {
        string res = buf2.retrieveMessage();
        cout << "补齐数据后：hasMessage=true, msg=" << res << endl;
    }

    return 0;
}