#ifndef _COMMANDSCANNER_H
#define _COMMANDSCANNER_H

#include <vector>
#include <map>
typedef enum {
  CTOKEN_NEUTRAL, CTOKEN_FORWARD, CTOKEN_BACK, CTOKEN_UP, CTOKEN_DOWN,
  CTOKEN_UPFORWARD, CTOKEN_UPBACK, CTOKEN_DOWNFORWARD, CTOKEN_DOWNBACK,
  CTOKEN_LP, CTOKEN_LK, CTOKEN_MP, CTOKEN_MK,
  CTOKEN_NUMBER,

  CTOKEN_RELEASED, CTOKEN_HELD,

  CTOKEN_AND, CTOKEN_OR,

  CTOKEN_ANY, 
  CTOKEN_NOT,
  CTOKEN_DELIM,
  CTOKEN_END,

} CommandTokenType;

struct CommandToken {
  CommandTokenType type;
  const char* start;
  uint8_t length;
};

class CommandScanner {
public:
  CommandScanner();
  ~CommandScanner();

  std::vector<CommandToken> scan(const char* inputString);

  CommandTokenType getInputType();
  CommandToken makeToken(CommandTokenType type);
  bool isAtEnd();
  char peek();
  char peekNext();
  char advance();
  bool match(char expected);
  void skipWhitespace();
  bool isAlpha(char c);
  bool isDigit(char c);
  CommandTokenType checkKeyword(int start, int end, const char* rest, CommandTokenType type);

  std::map<CommandTokenType, const char*> tokenToString = {
    {CTOKEN_NEUTRAL, "NEUTRAL"},
    {CTOKEN_DOWN, "DOWN"},
    {CTOKEN_FORWARD, "FORWARD"},
    {CTOKEN_BACK, "BACK"},
    {CTOKEN_UP, "UP"},
    {CTOKEN_DOWNBACK, "DOWNBACK"},
    {CTOKEN_DOWNFORWARD, "DOWNFORWARD"},
    {CTOKEN_UPBACK, "UPBACK"},
    {CTOKEN_UPFORWARD, "UPFORWARD"},
    {CTOKEN_LP, "LP"},
    {CTOKEN_LK, "LK"},
    {CTOKEN_MP, "MP"},
    {CTOKEN_MK, "MK"},
    {CTOKEN_NUMBER, "NUMBER"},
    {CTOKEN_RELEASED, "RELEASED"},
    {CTOKEN_HELD, "HELD"},
    {CTOKEN_AND, "AND"},
    {CTOKEN_OR, "OR"},
    {CTOKEN_ANY, "ANY"},
    {CTOKEN_NOT, "NOT"},
    {CTOKEN_DELIM, "DELIM"},
    {CTOKEN_END, "END"},
  };

private:
  const char* scannerStart;
  const char* scannerCurrent;
};

#endif 
