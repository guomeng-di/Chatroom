#include "TerminalInput.h"

#include <iostream>
#include <string>

using namespace std;


string TerminalInput::getInput()
{
    string input;

    getline(
        cin,
        input
    );

    return input;
}