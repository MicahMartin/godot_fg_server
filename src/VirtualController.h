#ifndef _VIRTUALCONTROLLER_H
#define _VIRTUALCONTROLLER_H

#include "CommandCompiler.h"
#include "circular_buffer.h"
#include <list>
#include <map>

typedef enum {
  NOINPUT = 0,

  RIGHT = 0x1,
  LEFT = 0x2,
  UP = 0x4,
  DOWN = 0x8,

  LP  = 0x10,
  MP = 0x20,
  HP = 0x40,
  AP = 0x80,

  LK  = 0x100,
  MK = 0x200,
  HK = 0x400,
  AK = 0x800,

  START = 0x1000,
  SELECT = 0x2000,
  MISC1 = 0x4000,
  MISC2 = 0x8000,

  DOWNLEFT = (DOWN | LEFT),
  DOWNRIGHT = (DOWN | RIGHT),
  UPLEFT = (UP | LEFT),
  UPRIGHT = (UP | RIGHT),
} Input;

struct InputEvent {
  InputEvent(uint16_t inputBit, bool pressed): inputBit(inputBit), pressed(pressed){}
  InputEvent(){}
  ~InputEvent(){}

  uint16_t inputBit;
  bool pressed;
  bool valid = true;
};

struct VirtualControllerObj {
  int currentInputState;
  int prevInputState;
  CircularBuffer<std::list<InputEvent>> inputHistory{120};
};

class CommandCompiler;
class VirtualController {
  public:
    VirtualController ();
    ~VirtualController ();

    void update(uint16_t input);

    std::list<InputEvent> getInputList(int index);

    void initCommandCompiler(const char* path);
    bool wasPressed(Input input, bool strict = true, int index = 0, bool pressed = true);
    bool wasPressedBuffer(Input input, bool strict = true, bool pressed = true);
    bool wasReleased(Input input, bool strict = true, int index = 0);
    bool isPressed(Input input, bool strict = true);
    bool checkCommand(int commandIndex, bool faceRight);

    inline bool wasPressedWrapper(Input input, bool strict, int index, bool faceRight){
      Input relativeInput = inputEnumMap[input](faceRight);
      return wasPressed(relativeInput, strict, index);
    };
    inline bool wasReleasedWrapper(Input input, bool strict, int index, bool faceRight){
      Input relativeInput = inputEnumMap[input](faceRight);
      return wasReleased(relativeInput, strict, index);
    };
    inline bool isPressedWrapper(Input input, bool strict, int index, bool faceRight){
      strict = false;
      Input relativeInput = inputEnumMap[input](faceRight);
      return isPressed(relativeInput, strict);
    };

    VirtualControllerObj saveState();
    void loadState(VirtualControllerObj stateObj);

    uint16_t currentInputState;
    uint16_t prevInputState;

    static std::map<Input, Input(*)(bool)> inputEnumMap;
    static std::map<int, Input(*)(bool)> inputMap;
    static std::map<Input, const char*> inputToString;
  private:
    CommandCompiler commandCompiler;
    CircularBuffer<std::list<InputEvent>> inputHistory{120};
};

#endif
