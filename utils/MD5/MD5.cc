//打开指定文件 → 分块读取全部内容 →
//用 OpenSSL 的 MD5 算法逐块计算 →
//最终输出一个 32 字符的十六进制 MD5 摘要字符串

#include "MD5.h"
#include <openssl/md5.h>
#include <fstream>
#include <iomanip>
#include <vector>
#include <sstream>

using namespace std;

string MD5::getFileMD5(const string& filepath){
    ifstream file(filepath,ios::binary);

    if(!file.is_open())
    {
        return "";
    }

    MD5_CTX md5;

    MD5_Init(&md5);

    vector<char> buffer(1024*1024*5);

    while(file.read(buffer.data(),buffer.size()))
    {
        MD5_Update(&md5,buffer.data(),file.gcount());
    }

    if(file.gcount()>0)
    {
        MD5_Update(&md5,buffer.data(),file.gcount());
    }

    unsigned char result[MD5_DIGEST_LENGTH];

    MD5_Final(result,&md5);

    stringstream ss;

    for(int i=0;i<MD5_DIGEST_LENGTH;i++)
    {
        ss<<hex<<setw(2)<<setfill('0')<<(int)result[i];
    }

    return ss.str();
}