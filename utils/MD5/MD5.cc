//打开指定文件 → 分块读取全部内容 → 
//用 OpenSSL 的 MD5 算法逐块计算 →
// 最终输出一个 32 字符的十六进制 MD5 摘要字符串
#include "MD5.h"
#include <openssl/md5.h>
#include <fstream>
#include <iomanip>
#include <sstream>
using namespace std;
string MD5::getFileMD5(const string& filepath){
    ifstream file(filepath,ios::binary);//找到并打开文件,定位到起始位置
    if(!file.is_open()) return "";
    MD5_CTX md5;//存计算过程中的数据
    MD5_Init(&md5);//固定的4个32位初始值(小端)
    char buffer[4096];
    while(file.read(buffer,sizeof(buffer)))  MD5_Update(&md5,buffer,file.gcount());
    if(file.gcount()>0) MD5_Update(&md5,buffer,file.gcount());
    
    unsigned char result[MD5_DIGEST_LENGTH];//128位(16字节)
    MD5_Final(result,&md5);
    stringstream ss;
    for(int i=0;i<MD5_DIGEST_LENGTH;i++)
        ss<<hex<<setw(2)<<setfill('0')<<(int)result[i];
//hex:16进制
    return ss.str();
}