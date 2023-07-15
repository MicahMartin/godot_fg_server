#ifndef _Stack_h
#define _Stack_h

#include "Common.h"
#include "Value.h"

class Stack {
public:
  Stack();
  ~Stack();
  void reset();

  void push(Value value);
  Value pop();
  Value peek(int distance);
  Value at(int index);
  void set(Value val, int index);

  Value stack[256];
  Value* stackTop;
};

#endif /* STACK_H */
