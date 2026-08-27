 #include "ChatInputMode.h"
 #include <iostream>
 #include <cstring>
 using namespace std;
 ChatInputMode::ChatInputMode(){
  if(tcgetattr(STDIN_FILENO,&oldTermios_)<0){
   return;
  }
  oldFlags_=fcntl(STDIN_FILENO,F_GETFL,0);
  if(oldFlags_<0){
   return;
  }
  termios current=oldTermios_;
  current.c_lflag&=~ICANON;
  current.c_lflag&=~ECHO;
  current.c_lflag&=~ECHONL;
  current.c_lflag&=~IEXTEN;
  current.c_cc[VMIN]=1;
  current.c_cc[VTIME]=0;
  if(tcsetattr(STDIN_FILENO,TCSANOW,&current)<0){
   return;
  }
  termiosChanged_=true;
  if(fcntl(STDIN_FILENO,F_SETFL,oldFlags_|O_NONBLOCK)<0){
   tcsetattr(STDIN_FILENO,TCSANOW,&oldTermios_);
   termiosChanged_=false;
   return;
  }
  flagsChanged_=true;
  const char* enablePaste="\033[?2004h";
  write(STDOUT_FILENO,enablePaste,strlen(enablePaste));
 }
 ChatInputMode::~ChatInputMode(){
  const char* disablePaste="\033[?2004l";
  write(STDOUT_FILENO,disablePaste,strlen(disablePaste));
  if(flagsChanged_&&oldFlags_>=0){
   fcntl(STDIN_FILENO,F_SETFL,oldFlags_);
  }
  if(termiosChanged_){
   tcsetattr(STDIN_FILENO,TCSANOW,&oldTermios_);
  }
 }
 bool ChatInputMode::active() const{
  return termiosChanged_&&flagsChanged_;
 }