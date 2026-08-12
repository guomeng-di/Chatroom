#include "EmailManager.h"


int main()
{

    EmailManager::instance()
    .sendCode(
        "2381793630@qq.com",
        "123456"
    );


    return 0;
}