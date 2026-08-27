 #pragma once
 #include <string>
 #include <deque>
 #include <mutex>
 class ChatInputReader{
 public:
  explicit ChatInputReader(bool splitMode=false);
  ~ChatInputReader();
  bool hasMessage() const;
  bool readMessage(std::string& message);
  void setSplitMode(bool splitMode);
  bool isSplitMode() const;
  void redraw();
 private:
  void parse();
  bool parseEscape();
  bool parsePasteBegin();
  bool parsePasteEnd();
  void appendCharacter(char ch);
  void appendNewline();
  void finishLine();
  void insertCharacter(char ch);
  void erasePreviousCharacter();
  void eraseNextCharacter();
  void moveCursorLeft();
  void moveCursorRight();
  void moveCursorHome();
  void moveCursorEnd();
  void submitCurrent();
  void submitCurrentWithoutNewline();
  void redrawInput();
  void redrawSimple();
  size_t previousUtf8Character() const;
  size_t nextUtf8Character() const;
  bool isUtf8Continuation(unsigned char c) const;
  void pushMessage(std::string message);
 private:
  bool splitMode_=false;
  bool pasteMode_=false;
  bool need_redraw_=false; //是否需要重绘标记
  std::string pending_;
  std::string current_;
  size_t cursor_=0;
  std::deque<std::string> messages_;
  mutable std::mutex mutex_;
  std::string prompt_="> ";
 };
 bool handleModeCommand(const std::string& message,ChatInputReader& inputReader);