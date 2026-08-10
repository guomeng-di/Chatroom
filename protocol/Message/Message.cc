#include "Message.h"
using namespace std;

Message::Message():length_(0){

}
Message::~Message(){

}
void Message::setBody(const std::string& body){
    body_=body;
    length_=body.size();
}
int Message::length(){
    return length_;
}
string Message::body(){
    return body_;
}