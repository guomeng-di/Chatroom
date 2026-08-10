#include "VerifyCode.h"
#include <cstdlib>
#include <ctime>

using namespace std;
string VerifyCode::generate(){
    srand(time(nullptr));
    int code=100000+rand()%900000;
    return to_string(code);
}