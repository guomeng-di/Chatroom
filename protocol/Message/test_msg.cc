#include "Message.h"
#include <iostream>
using namespace std;

int main()
{
    Message msg;
    msg.setBody("hello");

    cout << msg.length() << endl;
    cout << msg.body() << endl;

    return 0;
}