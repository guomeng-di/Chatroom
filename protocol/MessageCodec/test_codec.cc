#include "MessageCodec.h"
#include <iostream>
using namespace std;

int main()
{
    string s = "hello";

    // 编码：字符串 -> [4字节头][hello]
    string data = MessageCodec::encode(s);

    // 解码：完整包还原字符串
    string msg = MessageCodec::decode(data);

    cout << msg << endl;
    return 0;
}