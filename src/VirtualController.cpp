#include <godot_cpp/variant/utility_functions.hpp>
#include <bitset>
#include "VirtualController.h"
#include "CommandCompiler.h"

std::map<int, Input(*)(bool)> VirtualController::inputMap = {
  {1, [](bool faceRight) {return faceRight ? DOWNLEFT : DOWNRIGHT; }},
  {2, [](bool faceRight) {return DOWN; }},
  {3, [](bool faceRight) {return faceRight ? DOWNRIGHT : DOWNLEFT; }},
  {4, [](bool faceRight) {return faceRight ? LEFT : RIGHT; }},
  {5, [](bool faceRight) {return NOINPUT; }},
  {6, [](bool faceRight) {return faceRight ? RIGHT : LEFT; }},
  {7, [](bool faceRight) {return faceRight ? UPLEFT : UPRIGHT; }},
  {8, [](bool faceRight) {return UP; }},
  {9, [](bool faceRight) {return faceRight ? UPRIGHT : UPLEFT; }},
  {10, [](bool faceRight) {return LP; }},
  {11, [](bool faceRight) {return LK; }},
  {12, [](bool faceRight) {return MP; }},
  {13, [](bool faceRight) {return MK; }},
};

std::map<Input, Input(*)(bool)> VirtualController::inputEnumMap = {
  {RIGHT, [](bool faceRight) {return faceRight ? RIGHT : LEFT; }},
  {LEFT, [](bool faceRight) {return faceRight ? LEFT : RIGHT; }},
  {DOWNRIGHT, [](bool faceRight) {return faceRight ? DOWNRIGHT : DOWNLEFT; }},
  {DOWNLEFT, [](bool faceRight) {return faceRight ? DOWNLEFT : DOWNRIGHT; }},
  {UPRIGHT, [](bool faceRight) {return faceRight ? UPRIGHT : UPLEFT; }},
  {UPLEFT, [](bool faceRight) {return faceRight ? UPLEFT : UPRIGHT; }},
  {NOINPUT, [](bool faceRight) {return NOINPUT; }},
  {UP, [](bool faceRight) {return UP; }},
  {DOWN, [](bool faceRight) {return DOWN; }},
  {LP, [](bool faceRight) {return LP; }},
  {LK, [](bool faceRight) {return LK; }},
  {MP, [](bool faceRight) {return MP; }},
  {MK, [](bool faceRight) {return MK; }},
};

std::map<Input, const char*> VirtualController::inputToString = {
  {NOINPUT, "NEUTRAL"},
  {DOWN, "DOWN"},
  {RIGHT, "RIGHT"},
  {LEFT, "LEFT"},
  {UP, "UP"},
  {DOWNLEFT, "DOWNLEFT"},
  {DOWNRIGHT, "DOWNRIGHT"},
  {UPLEFT, "UPLEFT"},
  {UPRIGHT, "UPRIGHT"},
  {LP, "LIGHT_P"},
  {MP, "MEDIUM_P"},
  {HP, "HEAVY_P"},
  {AP, "ALL_P"},
  {LK, "LIGHT_K"},
  {MK, "MEDIUM_K"},
  {HK, "HEAVY_K"},
  {AK, "ALL_K"},
};

VirtualController::VirtualController() {
  // stateObj.currentInputState = 0;
  // stateObj.prevInputState = 0;
  currentInputState = 0;
  prevInputState = 0;
  isRecording = false;
  isPlayback = false;
  for (int i = 0; i < 120; i++) {
    inputHistory.push_back(std::list<InputEvent>());
  }
}

VirtualController::~VirtualController() {
  // delete commandCompiler;
}

void VirtualController::update(uint16_t input){
  // use gamestate 
  // stateObj.inputHistory.push_back(std::list<InputEvent>());
  if(isRecording){
    recording.push_back(input);
  } else if(isPlayback){
    if(recordIterator == recording.end()){
      togglePlayback();
    } else {
      input = *recordIterator;
      ++recordIterator;
    }
  }
  inputHistory.push_back(std::list<InputEvent>());

  prevInputState = currentInputState;
  uint8_t prevStickState = currentInputState & 0x0F;

  currentInputState = input;
  uint8_t currentStickState = input & 0x0F;

  // Cardinal Events
  if (prevStickState != currentStickState) {
    if (prevStickState == 0) {
      // godot::UtilityFunctions::print("Neutral Release");
      inputHistory.back().push_back(InputEvent(NOINPUT, false));
    } else {
      // godot::UtilityFunctions::print(inputToString[Input(prevStickState)], " Release");
      inputHistory.back().push_back(InputEvent(prevStickState, false));
    }

    if (currentStickState == 0) {
      // godot::UtilityFunctions::print("Neutral Press");
      inputHistory.back().push_back(InputEvent(NOINPUT, true));
    } else {
      // godot::UtilityFunctions::print(inputToString[Input(currentStickState)], " Press");
      inputHistory.back().push_back(InputEvent(currentStickState, true));
    }

  }



  // Button Presses
  std::bitset<16> prevButtonState(prevInputState);
  std::bitset<16> currentButtonState(currentInputState);
  for (int i = 4; i < 10; ++i) {
    uint16_t mask = 0;
    mask |= (1 << i);
    if (currentButtonState.test(i) && !prevButtonState.test(i)) {
      // godot::UtilityFunctions::print(inputToString[Input(mask)], " Press");
      inputHistory.back().push_back(InputEvent(mask, true));
    } else if (!currentButtonState.test(i) && prevButtonState.test(i)) {
      // godot::UtilityFunctions::print(inputToString[Input(mask)], " Release");
      inputHistory.back().push_back(InputEvent(mask, false));
    }
  }
}

void VirtualController::initCommandCompiler(const char* path) { 
  commandCompiler.controllerPointer = this;
  commandCompiler.init("/Users/martin/dev/godot_projects/template_test/GDExtensionTemplate/src/commands.json");
}

bool VirtualController::wasPressed(Input input, bool strict, int index, bool pressed) {
  int historySize = inputHistory.size();
  if (index >= historySize) {
    // godot::UtilityFunctions::print("not enough history");
    return false;
  }
  std::list<InputEvent> eventList = inputHistory[119 - index];
  if (eventList.empty()) {
    return false;
  }

  for (auto const& event : eventList) {
    if (event.valid) {
      if ((pressed && event.pressed) || (!pressed && !event.pressed)) {
        if (input <= 10 && strict) {
          return (input == (event.inputBit & 0x0F));
        }
        else if (event.inputBit & input) {
          return true;
        }
      }
    }
  }
  // printf("wasnt pressed\n");
  return false;
}

bool VirtualController::wasPressedBuffer(Input input, bool strict, bool pressed) {
  int buffLen = 4;
  bool found = false;
  int historySize = inputHistory.size();

  for (int i = 0; i < buffLen && !found; i++) {
    std::list<InputEvent> eventList = inputHistory[119 - i];

    if (eventList.size() > 0) {
      for (auto const& event : eventList) {
        if (event.valid) {
          if ((pressed && event.pressed) || (!pressed && !event.pressed)) {
            if (input <= 10 && strict) {
              found = (input == (event.inputBit & 0x0F));
            }
            else if (event.inputBit & input) {
              // printf("checking non cardinal direction %d\n", input);
              // printf("was pressed\n");
              found = true;
            }
          }
        }
      }
    }
  }

  return found;
}

bool VirtualController::wasReleased(Input input, bool strict, int index) {
  return wasPressed(input, strict, index, false);
}

bool VirtualController::isPressed(Input input, bool strict) {
  if (input < 16 && strict) {
    return (input == (currentInputState & 0x0F));
  }
  else {
    return (currentInputState & input);
  }
}

bool VirtualController::checkCommand(int commandIndex, bool faceRight) {
  bool foundPart = false;
  bool foundCommand = false;
  bool breakFlag = false;
  bool firstFind = true;
  int searchOffset = 0;
  int firstFindOffet = 0;

  if (commandIndex >= commandCompiler.commands.size()) {
    return false;
  }

  std::vector<CommandNode>* command = &commandCompiler.commands[commandIndex].command;
  bool clears = commandCompiler.commands[commandIndex].clears;
  for (int i = command->size() - 1; i >= 0 && !breakFlag; i--) {
    CommandNode& funcNode = (*command)[i];

    for (int x = 0; ((!foundPart) && (x < funcNode.bufferLength)); x++) {
      foundPart = (funcNode.function)(x + searchOffset, faceRight);
      if (foundPart) {
        // godot::UtilityFunctions::print("Found part: ", x);
        if (firstFind) {
          firstFindOffet = x;
          firstFind = false;
        }
        searchOffset += x;
      }
    }

    if (foundPart) {
      foundPart = false;
      if (i == 0) {
        foundCommand = true;
        godot::UtilityFunctions::print("Found Command: ", commandIndex);
        printf("found command:%d\n", commandIndex);
        if (clears) {
          for(int y = firstFindOffet; y < inputHistory.size(); y++) {
            std::list<InputEvent>* eventList = &inputHistory[119 - y];

            if (eventList->size() > 0) {
              for (auto& event : *eventList) {
                event.valid = false;
              }
            }
          }
          // inputHistory.erase(inputHistory.begin() + firstFindOffet, inputHistory.end());
        }
      }
    }
    else {
      breakFlag = true;
    }
  }

  return foundCommand;
}

VirtualControllerObj VirtualController::saveState(){
  VirtualControllerObj stateObj;
  stateObj.prevInputState = prevInputState;
  stateObj.currentInputState = currentInputState;
  for (int i = 0; i < 120; i++) {
    InputFrame currentFrame;
    for(auto& inputEvent : inputHistory[i]) {
      currentFrame.events[currentFrame.numEvents] = inputEvent;
      currentFrame.numEvents++;
    }
    stateObj.flatHistory[i] = currentFrame;
  }
  return stateObj;
}

void VirtualController::loadState(VirtualControllerObj stateObj){
  prevInputState = stateObj.prevInputState;
  currentInputState = stateObj.currentInputState;
  inputHistory.clear();
  for (int i = 0; i < 120; i++) {
    inputHistory.push_back(std::list<InputEvent>());
    InputFrame currentFrame = stateObj.flatHistory[i];
    for (int x = 0; x < currentFrame.numEvents; x++) {
      inputHistory[i].push_back(currentFrame.events[x]);
    }
  }
}

void VirtualController::toggleRecording(){
  if(isPlayback){
    return;
  }
  if(isRecording){
    isRecording = false;
    godot::UtilityFunctions::print("Recording stopped, size:", int(recording.size()));
  } else {
    recording.clear();
    isRecording = true;
    godot::UtilityFunctions::print("Recording started");
  }
}

void VirtualController::togglePlayback(){
  if(isRecording){
    return;
  }
  if(isPlayback){
    isPlayback = false;
    godot::UtilityFunctions::print("Playback stopped");
  } else {
    isPlayback = true;
    recordIterator = recording.begin();
    godot::UtilityFunctions::print("Playback started");
  }
}
