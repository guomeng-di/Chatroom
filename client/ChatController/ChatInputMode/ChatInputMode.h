 #pragma once
 #include <termios.h>
 #include <unistd.h>
 #include <fcntl.h>
 #include <string>
 class ChatInputMode{
 public:
  ChatInputMode();
  ~ChatInputMode();
  bool active() const;
 private:
  termios oldTermios_{};
  int oldFlags_=-1;
  bool termiosChanged_=false;
  bool flagsChanged_=false;
 };