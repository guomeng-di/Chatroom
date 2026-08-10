#include "JsonProtocol.h"
#include <stdexcept>
#include <iostream>
#include <exception>
#include <string>  
#include "../../netlib/base/Logger.h"
using namespace std;
std::string JsonProtocol::encode(const json& js){
    return js.dump();
}
json JsonProtocol::decode(const std::string& msg){
    //return json::parse(msg);
    try{
        return json::parse(msg);
    }
    catch(const exception& e){
        Logger::instance().error(string("json parse error: ") + e.what());
        return json();
    }
}