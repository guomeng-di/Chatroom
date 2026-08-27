 #include "ChatInputReader.h"
 #include <unistd.h>
 #include <fcntl.h>
 #include <sys/select.h>
 #include <iostream>
 #include <algorithm>
 #include <cstring>
 #include <cerrno>
 using namespace std;
 ChatInputReader::ChatInputReader(bool splitMode):splitMode_(splitMode){
  redrawInput();
 }
 ChatInputReader::~ChatInputReader(){
  cout<<"\033[0m"<<endl;
  cout.flush();
 }
 bool ChatInputReader::hasMessage() const{
  lock_guard<mutex> lock(mutex_);
  return !messages_.empty();
 }
 void ChatInputReader::setSplitMode(bool splitMode){
  splitMode_=splitMode;
  need_redraw_ = true;
 }
 bool ChatInputReader::isSplitMode() const{
  return splitMode_;
 }
 bool ChatInputReader::readMessage(string& message){
  char buf[4096];
  need_redraw_ = false;
  while(true){
   ssize_t len=read(STDIN_FILENO,buf,sizeof(buf));
   if(len>0){
    pending_.append(buf,static_cast<size_t>(len));
    parse();
   }else if(len<0){
    if(errno==EINTR){
     continue;
    }
    if(errno!=EAGAIN&&errno!=EWOULDBLOCK){
     return false;
    }
   }else if(len==0){
    return false;
   }
   break;
  }
  if(need_redraw_){
        redrawInput();
    }
  lock_guard<mutex> lock(mutex_);
  if(messages_.empty()){
   return false;
  }
  message=move(messages_.front());
  messages_.pop_front();
  return true;
 }
 void ChatInputReader::pushMessage(string message){
  lock_guard<mutex> lock(mutex_);
  messages_.push_back(move(message));
 }
 void ChatInputReader::finishLine(){
  string message=move(current_);
  current_.clear();
  cursor_=0;
  if(!message.empty()){
   pushMessage(move(message));
  }
  need_redraw_ = true;
 }
 void ChatInputReader::submitCurrent(){
  finishLine();
 }
 void ChatInputReader::submitCurrentWithoutNewline(){
  if(current_.empty()){
   need_redraw_ = true;
   return;
  }
  finishLine();
 }
 void ChatInputReader::appendNewline(){
  if(splitMode_){
   submitCurrent();
   return;
  }
  insertCharacter('\n');
 }
 void ChatInputReader::appendCharacter(char ch){
  if(pasteMode_){
   if(ch=='\r'){
    if(splitMode_){
     submitCurrent();
    }else{
     insertCharacter('\n');
    }
    need_redraw_ = true;
    return;
   }
   if(ch=='\n'){
    if(splitMode_){
     submitCurrent();
    }else{
     insertCharacter('\n');
    }
    need_redraw_ = true;
    return;
   }
  }
  if(ch=='\r'){
   appendNewline();
   need_redraw_ = true;
   return;
  }
  if(ch=='\n'){
   appendNewline();
   need_redraw_ = true;
   return;
  }
  if(ch==127||ch==8){
   erasePreviousCharacter();
   need_redraw_ = true;
   return;
  }
  if(ch==3){
   current_.clear();
   cursor_=0;
   cout<<"^C"<<endl;
   need_redraw_ = true;
   return;
  }
  if(ch==4){
   if(current_.empty()){
    pushMessage("quit");
    need_redraw_ = true;
    return;
   }
   eraseNextCharacter();
   need_redraw_ = true;
   return;
  }
  if(static_cast<unsigned char>(ch)<32){
   return;
  }
  insertCharacter(ch);
  need_redraw_ = true;
 }
 bool ChatInputReader::parsePasteBegin(){
  static const string begin="\033[200~";
  if(pending_.size()<begin.size()){
   return false;
  }
  if(pending_.compare(0,begin.size(),begin)==0){
   pending_.erase(0,begin.size());
   pasteMode_=true;
   return true;
  }
  return false;
 }
 bool ChatInputReader::parsePasteEnd(){
  static const string end="\033[201~";
  if(pending_.size()<end.size()){
   return false;
  }
  if(pending_.compare(0,end.size(),end)==0){
   pending_.erase(0,end.size());
   pasteMode_=false;
   need_redraw_ = true;
   return true;
  }
  return false;
 }
 bool ChatInputReader::parseEscape(){
  if(pending_.empty()||pending_[0]!=27){
   return false;
  }
  if(pending_.size()==1){
   return false;
  }
  if(pending_[1]!='['){
   pending_.erase(0,1);
   return true;
  }
  if(pending_.size()<3){
   return false;
  }
  char command=pending_[2];
  if(command=='A'){
   pending_.erase(0,3);
   if(!current_.empty()){
    submitCurrentWithoutNewline();
   }
   return true;
  }
  if(command=='B'){
   pending_.erase(0,3);
   return true;
  }
  if(command=='C'){
   pending_.erase(0,3);
   moveCursorRight();
   return true;
  }
  if(command=='D'){
   pending_.erase(0,3);
   moveCursorLeft();
   return true;
  }
  if(command=='H'){
   pending_.erase(0,3);
   moveCursorHome();
   return true;
  }
  if(command=='F'){
   pending_.erase(0,3);
   moveCursorEnd();
   return true;
  }
  if(pending_.size()>=4&&pending_[2]=='3'&&pending_[3]=='~'){
   pending_.erase(0,4);
   eraseNextCharacter();
   return true;
  }
  if(pending_.size()>=4&&pending_[2]=='1'&&pending_[3]=='~'){
   pending_.erase(0,4);
   moveCursorHome();
   return true;
  }
  if(pending_.size()>=4&&pending_[2]=='4'&&pending_[3]=='~'){
   pending_.erase(0,4);
   moveCursorEnd();
   return true;
  }
  return false;
 }
 void ChatInputReader::parse(){
    need_redraw_=0;
  while(!pending_.empty()){
   if(parsePasteBegin()){
    continue;
   }
   if(parsePasteEnd()){
    need_redraw_=1;
    continue;
   }
   if(!pasteMode_&&pending_[0]==27){
    if(parseEscape()){
        need_redraw_=1;
     continue;
    }
    if(pending_.size()<2){
     break;
    }
   }
   char ch=pending_[0];
   pending_.erase(0,1);
   appendCharacter(ch);
   need_redraw_=1;
  }
 }
 void ChatInputReader::insertCharacter(char ch){
  current_.insert(cursor_,1,ch);
  cursor_++;
  need_redraw_=1;
 }
 bool ChatInputReader::isUtf8Continuation(unsigned char c) const{
  return (c&0xC0)==0x80;
 }
 size_t ChatInputReader::previousUtf8Character() const{
  if(cursor_==0){
   return 0;
  }
  size_t pos=cursor_-1;
  while(pos>0&&isUtf8Continuation(static_cast<unsigned char>(current_[pos]))){
   pos--;
  }
  return pos;
 }
 size_t ChatInputReader::nextUtf8Character() const{
  if(cursor_>=current_.size()){
   return current_.size();
  }
  size_t pos=cursor_+1;
  while(pos<current_.size()&&isUtf8Continuation(static_cast<unsigned char>(current_[pos]))){
   pos++;
  }
  return pos;
 }
 void ChatInputReader::erasePreviousCharacter(){
  if(cursor_==0){
   return;
  }
  size_t begin=previousUtf8Character();
  current_.erase(begin,cursor_-begin);
  cursor_=begin;
  need_redraw_ = true;
 }
 void ChatInputReader::eraseNextCharacter(){
  if(cursor_>=current_.size()){
   return;
  }
  size_t end=nextUtf8Character();
  current_.erase(cursor_,end-cursor_);
  need_redraw_ = true;
 }
 void ChatInputReader::moveCursorLeft(){
  if(cursor_==0){
   return;
  }
  cursor_=previousUtf8Character();
  need_redraw_ = true;
 }
 void ChatInputReader::moveCursorRight(){
  if(cursor_>=current_.size()){
   return;
  }
  cursor_=nextUtf8Character();
  need_redraw_ = true;
 }
 void ChatInputReader::moveCursorHome(){
  cursor_=0;
  need_redraw_ = true;
 }
 void ChatInputReader::moveCursorEnd(){
  cursor_=current_.size();
  need_redraw_ = true;
 }
 void ChatInputReader::redraw(){
  redrawInput();
 }
 void ChatInputReader::redrawSimple(){
  cout<<"\r\033[2K";
  cout<<prompt_;
  cout<<current_;
  cout.flush();
  if(cursor_<current_.size()){
   size_t right=current_.size()-cursor_;
   cout<<"\033["<<right<<"D";
  }
 }
 void ChatInputReader::redrawInput(){
    cout << "\033[s";
  cout<<"\r\033[2K";
  cout<<prompt_;
  if(current_.empty()){
   cout.flush();
   cout << "\033[u";
   return;
  }
  size_t cursorLineStart=0;
  size_t cursorColumn=0;
  size_t i=0;
  while(i<cursor_){
   if(current_[i]=='\n'){
    cursorLineStart=i+1;
    cursorColumn=0;
   }else{
    cursorColumn++;
   }
   i++;
  }
  cout<<current_;
  size_t totalNewlines=0;
  for(char c:current_){
   if(c=='\n'){
    totalNewlines++;
   }
  }
  size_t cursorNewlines=0;
  for(size_t p=0;p<cursor_;p++){
   if(current_[p]=='\n'){
    cursorNewlines++;
   }
  }
  if(totalNewlines>cursorNewlines){
   size_t up=totalNewlines-cursorNewlines;
   cout<<"\033["<<up<<"A";
  }
  size_t endColumn=0;
  size_t p=current_.size();
  while(p>0&&current_[p-1]!='\n'){
   endColumn++;
   p--;
  }
  if(endColumn>cursorColumn){
   cout<<"\033["<<(endColumn-cursorColumn)<<"D";
  }
  cout << "\033 [J";
  cout << "\033 [u";
  cout.flush();
 }
 bool handleModeCommand(const string& message,ChatInputReader& inputReader){
  if(message=="/mode 1"){
   inputReader.setSplitMode(false);
   cout<<endl<<"\033[32m"<<"已切换为整体发送,输入换行不会发送,按方向键↑发送"<<"\033[0m"<<endl;
   inputReader.redraw();
   return true;
  }
  if(message=="/mode 2"){
   inputReader.setSplitMode(true);
   cout<<endl<<"\033[32m"<<"已切换为分行发送,按回车立即发送"<<"\033[0m"<<endl;
   inputReader.redraw();
   return true;
  }
  if(message=="/mode"){
   cout<<endl<<"\033[33m";
   cout<<"当前发送方式:"<<(inputReader.isSplitMode()?"分行发送":"整体发送")<<endl;
   cout<<"输入 /mode 1 切换为整体发送"<<endl;
   cout<<"输入 /mode 2 切换为分行发送"<<endl;
   cout<<"\033[0m";
   inputReader.redraw();
   return true;
  }
  return false;
 }