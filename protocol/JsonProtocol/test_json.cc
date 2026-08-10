#include "JsonProtocol.h"
#include <iostream>
using namespace std;

int main()
{
    // ① 创建空json对象
    json js;
    js["msgid"]=1;
    js["name"]="jack";
    js["password"]="123456";

    // ② 编码
    string msg = JsonProtocol::encode(js);

    // ③ 打印msg（二进制报文，控制台只会显示后面的JSON字符串，看不见前面4字节长度）
    cout<<msg<<endl;

    // ④ 解码
    json js2 = JsonProtocol::decode(msg);

    // ⑤ 取出字段打印
    cout<<js2["name"]<<endl;
}