#ifndef _COMMANDCOMPILER_H
#define _COMMANDCOMPILER_H

#include <string>
#include <vector>
#include <functional>
#include "CommandScanner.h"

typedef std::function<bool(int, bool)> CommandFunction;
struct CommandNode {
  CommandFunction function;
  int bufferLength;
};

struct CommandObj {
  std::vector<CommandNode> command;
  bool clears;
};

struct CommandStringObj {
  std::string command;
  bool clears;
};

class VirtualController;
class CommandCompiler {
public:

  CommandCompiler();
  ~CommandCompiler();

  void init(const char* path);
  void compile(const char* inputString, bool clears);

  CommandNode compileNode();
  CommandNode compileOneNode();
  CommandFunction binaryCommand(CommandFunction currentFunc, CommandTokenType type);

  std::vector<CommandStringObj> commandStrings;
  std::vector<CommandObj> commands;
  VirtualController* controllerPointer;
private:
  CommandScanner commandScanner;
  CommandToken* currentToken;
};

#endif /* _CommandCompiler_h */
