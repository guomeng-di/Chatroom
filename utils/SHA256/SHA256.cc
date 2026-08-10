#include "SHA256.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

std::string HashSHA256::encode(const std::string& str)
{
    unsigned char buf[SHA256_DIGEST_LENGTH];
    // 全局域::SHA256，强制调用openssl库函数，避开类名冲突
    ::SHA256(reinterpret_cast<const unsigned char*>(str.data()), str.size(), buf);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(buf[i]);
    }
    return ss.str();
}