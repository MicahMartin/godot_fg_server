#include <fstream>
#include <godot_cpp/variant/utility_functions.hpp>
#include "json.hpp"
#include "CommandCompiler.h"
#include "VirtualController.h"

// each 'commandString' is a descriptor for a stack of boolean function calls
// P | ~P = ((wasPressed(LP)) || (wasReleased(LP)))
// @F & !D = ((wasPressed(F, strict = false)) && !(wasPressed(D)))
// MP & *D = ((wasPressed(MP)) && (isPressed(F)))
// DF = (wasPressed(DF))
// ~D = (wasReleased(D))

//  Forward, neutral, forward
//  "F, N, F",
//  back , neutral, back 
//  "B, N, B",
//  any down release, neutral, any down press, LP
//  "@~D, N, @D, LP",
//  any forward that doesnt include down, neutral, forward (lienent dash)
//  "@F & !D, N, F"
//  any back that doesnt include down, neutral, forward (lienent backdash)
//  "@B & !D, N, B",
//  release of down, downforward, anyforward that doesnt include down, lk or release of lk (236K)
//  "~D, DF, @F & !D, LK | ~LK",
//  release of down, downback, any back that doesnt include down, lp or release of lp (214P)
//  "~D, DB, @B & !D, LP | ~LP",
//  MP + forward IS pressed
//  "MP & *F",
//  MP + back IS pressed 
//  "MP & *B",
//
// input = N, F, B, U, D, UF, UB, DF, DB, LP, LK, MP, MK
// funcMods = ~, *, @
// unary = !
// binary = &, |
// TODO: unary and binary
//  "MP & *F",
//  "MP & *B",
//  "@F & !D, N, F"
//  "@B & !D, N, B",
//  "~D, DF, @F & !D, LK | ~LK",
//  "~D, DB, @B & !D, LP | ~LP",
//  TODO: load from file
//   "~D, 20DF, 20F, 8LP | 8~LP", // 214P
CommandCompiler::CommandCompiler() {
}

CommandCompiler::~CommandCompiler(){ }

void CommandCompiler::init(const char* path) {
  std::ifstream configFile(path);
  nlohmann::json commandJson;

  configFile >> commandJson;
  godot::UtilityFunctions::print("loaded command json\n");

  commandStrings.clear();
  commands.clear();

  for (auto& commandStringObj : commandJson["commands"].items()) {
    std::string commandString = commandStringObj.value()["command"].get<std::string>();
    bool clears = commandStringObj.value()["clears"].get<bool>();

    CommandStringObj command{ commandString, clears };

    commandStrings.push_back(command);
  }

  for (int i = 0; i < commandStrings.size(); ++i) {
    godot::UtilityFunctions::print("compiling command string: ", commandStrings[i].command.c_str());
    compile(commandStrings[i].command.c_str(), commandStrings[i].clears);
  }

  godot::UtilityFunctions::print("done compiling commands\n");
  printf("done compiling commands\n");
}

void CommandCompiler::compile(const char* inputString, bool clears) {
  // consume all tokens up until a delim
  // create annonymous boolean func commandFunc
  // get function pointers to wasPressed, wasReleased, and isPressed, 
  // set function pointer to wasPressed
  // set strict param to true
  // if we match the function mod any, set the strict param to false
  // if we match the function mod *, set the function pointer to isPressed
  // if we match the function mod ~, set the function pointer to wasReleased
  // bind strict param to function pointer
  // set command func's return to the evaluation of function @ function pointer
  // push command func onto commandFuncStack
  CommandObj commandObj;
  commandObj.clears = clears;

  std::vector<CommandToken> tokens = commandScanner.scan(inputString);
  currentToken = &tokens[0];

  while(currentToken->type != CTOKEN_END){
    godot::UtilityFunctions::print("new node start\n");
    commandObj.command.push_back(compileNode());
    godot::UtilityFunctions::print("new node end\n");
  }
  commands.push_back(commandObj);
}

CommandNode CommandCompiler::compileNode(){
  // function pointer is &VirtualController::wasPressed by default
  // bool strictness is true by default
  using namespace std::placeholders;
  std::function<bool(Input, bool, int, bool)> funcPointer = std::bind(&VirtualController::wasPressedWrapper, controllerPointer, _1, _2, _3, _4);
  CommandNode finalNode;
  CommandFunction& finalFunc = finalNode.function;
  bool strictness = true;
  finalNode.bufferLength = 8;

  while(currentToken->type != CTOKEN_DELIM && currentToken->type != CTOKEN_END) {
    switch (currentToken->type) {
      case CTOKEN_RELEASED: {
        funcPointer = std::bind(&VirtualController::wasReleasedWrapper, controllerPointer, _1, _2, _3, _4);
        godot::UtilityFunctions::print("setting func pointer to wasReleasedWrapper\n");
        currentToken++;
      }
      break;
      case CTOKEN_HELD: {
        funcPointer = std::bind(&VirtualController::isPressedWrapper, controllerPointer, _1, _2, _3, _4);
        godot::UtilityFunctions::print("setting func pointer to isPressedWrapper\n");
        currentToken++;
      }
      break;
      case CTOKEN_ANY: {
        strictness = false;
        godot::UtilityFunctions::print("setting strictness to false\n");
        currentToken++;
      }
      break;
      case CTOKEN_NEUTRAL: {
       finalFunc = std::bind(funcPointer, NOINPUT, strictness, _1, _2);
       godot::UtilityFunctions::print("building neutral\n");
       currentToken++;
      }
      break;
      case CTOKEN_FORWARD: {
       finalFunc = std::bind(funcPointer, RIGHT, strictness, _1, _2);
       godot::UtilityFunctions::print("building forward\n");
       currentToken++;
      }
      break;
      case CTOKEN_BACK: {
        finalFunc = std::bind(funcPointer, LEFT, strictness, _1, _2);
        godot::UtilityFunctions::print("building back\n");
        currentToken++;
      }
      break;
      case CTOKEN_UP: {
        finalFunc = std::bind(funcPointer, UP, strictness, _1, _2);
        godot::UtilityFunctions::print("building up\n");
        currentToken++;
      }
      break;
      case CTOKEN_DOWN: {
        finalFunc = std::bind(funcPointer, DOWN, strictness, _1, _2);
        godot::UtilityFunctions::print("building down\n");
        currentToken++;
      }
      break;
      case CTOKEN_UPFORWARD: {
        finalFunc = std::bind(funcPointer, UPRIGHT, strictness, _1, _2);
        godot::UtilityFunctions::print("building upforward\n");
        currentToken++;
      }
      break;
      case CTOKEN_UPBACK: {
        finalFunc = std::bind(funcPointer, UPLEFT, strictness, _1, _2);
        godot::UtilityFunctions::print("building upback\n");
        currentToken++;
      }
      break;
      case CTOKEN_DOWNFORWARD: {
        finalFunc = std::bind(funcPointer, DOWNRIGHT, strictness, _1, _2);
        godot::UtilityFunctions::print("building upforward\n");
        currentToken++;
      }
      break;
      case CTOKEN_DOWNBACK: {
        finalFunc = std::bind(funcPointer, DOWNLEFT, strictness, _1, _2);
        godot::UtilityFunctions::print("building downback\n");
        currentToken++;
      }
      break;
      case CTOKEN_LP: {
        finalFunc = std::bind(funcPointer, LP, strictness, _1, _2);
        godot::UtilityFunctions::print("building lightpunch\n");
        currentToken++;
      }
      break;
      case CTOKEN_LK: {
        finalFunc = std::bind(funcPointer, LK, strictness, _1, _2);
        godot::UtilityFunctions::print("building lightk\n");
        currentToken++;
      }
      break;
      case CTOKEN_MP: {
        finalFunc = std::bind(funcPointer, MP, strictness, _1, _2);
        godot::UtilityFunctions::print("building mediumP\n");
        currentToken++;
      }
      break;
      case CTOKEN_MK: {
        finalFunc = std::bind(funcPointer, MK, strictness, _1, _2);
        godot::UtilityFunctions::print("building mediumKick\n");
        currentToken++;
      }
      break;
      case CTOKEN_NUMBER: {
        finalNode.bufferLength = strtol(currentToken->start, NULL, 10);
        godot::UtilityFunctions::print("we got a number! %d\n", finalNode.bufferLength);
        currentToken++;
      }
      break;
      case CTOKEN_OR: {
        currentToken++;
        godot::UtilityFunctions::print("building or\n");
        finalFunc = binaryCommand(finalFunc, CTOKEN_OR);
        godot::UtilityFunctions::print("or control returned\n");
      }
      break;
      case CTOKEN_AND: {
        currentToken++;
        godot::UtilityFunctions::print("building and\n");
        finalFunc = binaryCommand(finalFunc, CTOKEN_AND);
      }
      break;
      default:
        break;
    }
  }
  if(currentToken->type != CTOKEN_END){
    currentToken++;
  }
  godot::UtilityFunctions::print(
      "The token pointer is at:", currentToken->start,
      "The length: ", currentToken->length,
      "The type: ", commandScanner.tokenToString[currentToken->type]
      );

  // CommandTokenType type;
  // const char* start;
  // uint8_t length;

  return finalNode;
}


CommandNode CommandCompiler::compileOneNode(){
  // function pointer is &VirtualController::wasPressed by default
  // bool strictness is true by default
  using namespace std::placeholders;
  std::function<bool(Input, bool, int, bool)> funcPointer = std::bind(&VirtualController::wasPressedWrapper, controllerPointer, _1, _2, _3, _4);
  CommandNode finalNode;
  CommandFunction& finalFunc = finalNode.function;
  bool strictness = true;
  finalNode.bufferLength = 8;

  while(currentToken->type != CTOKEN_DELIM && currentToken->type != CTOKEN_END) {
    switch (currentToken->type) {
      case CTOKEN_RELEASED: {
        funcPointer = std::bind(&VirtualController::wasReleasedWrapper, controllerPointer, _1, _2, _3, _4);
        godot::UtilityFunctions::print("setting func pointer to wasReleasedWrapper\n");
        currentToken++;
      }
      break;
      case CTOKEN_HELD: {
        funcPointer = std::bind(&VirtualController::isPressedWrapper, controllerPointer, _1, _2, _3, _4);
        godot::UtilityFunctions::print("setting func pointer to isPressedWrapper\n");
        currentToken++;
      }
      break;
      case CTOKEN_ANY: {
        strictness = false;
        godot::UtilityFunctions::print("setting strictness to false\n");
        currentToken++;
      }
      break;
      case CTOKEN_NEUTRAL: {
       finalFunc = std::bind(funcPointer, NOINPUT, strictness, _1, _2);
       godot::UtilityFunctions::print("building neutral\n");
       currentToken++;
      }
      break;
      case CTOKEN_FORWARD: {
       finalFunc = std::bind(funcPointer, RIGHT, strictness, _1, _2);
       godot::UtilityFunctions::print("building forward\n");
       currentToken++;
      }
      break;
      case CTOKEN_BACK: {
        finalFunc = std::bind(funcPointer, LEFT, strictness, _1, _2);
        godot::UtilityFunctions::print("building back\n");
        currentToken++;
      }
      break;
      case CTOKEN_UP: {
        finalFunc = std::bind(funcPointer, UP, strictness, _1, _2);
        godot::UtilityFunctions::print("building up\n");
        currentToken++;
      }
      break;
      case CTOKEN_DOWN: {
        finalFunc = std::bind(funcPointer, DOWN, strictness, _1, _2);
        godot::UtilityFunctions::print("building down\n");
        currentToken++;
      }
      break;
      case CTOKEN_UPFORWARD: {
        finalFunc = std::bind(funcPointer, UPRIGHT, strictness, _1, _2);
        godot::UtilityFunctions::print("building upforward\n");
        currentToken++;
      }
      break;
      case CTOKEN_UPBACK: {
        finalFunc = std::bind(funcPointer, UPLEFT, strictness, _1, _2);
        godot::UtilityFunctions::print("building upback\n");
        currentToken++;
      }
      break;
      case CTOKEN_DOWNFORWARD: {
        finalFunc = std::bind(funcPointer, DOWNRIGHT, strictness, _1, _2);
        godot::UtilityFunctions::print("building upforward\n");
        currentToken++;
      }
      break;
      case CTOKEN_DOWNBACK: {
        finalFunc = std::bind(funcPointer, DOWNLEFT, strictness, _1, _2);
        godot::UtilityFunctions::print("building downback\n");
        currentToken++;
      }
      break;
      case CTOKEN_LP: {
        finalFunc = std::bind(funcPointer, LP, strictness, _1, _2);
        godot::UtilityFunctions::print("building lightpunch\n");
        currentToken++;
      }
      break;
      case CTOKEN_LK: {
        finalFunc = std::bind(funcPointer, LK, strictness, _1, _2);
        godot::UtilityFunctions::print("building lightk\n");
        currentToken++;
      }
      break;
      case CTOKEN_MP: {
        finalFunc = std::bind(funcPointer, MP, strictness, _1, _2);
        godot::UtilityFunctions::print("building mediumP\n");
        currentToken++;
      }
      break;
      case CTOKEN_MK: {
        finalFunc = std::bind(funcPointer, MK, strictness, _1, _2);
        godot::UtilityFunctions::print("building mediumKick\n");
        currentToken++;
      }
      break;
      case CTOKEN_NUMBER: {
        finalNode.bufferLength = strtol(currentToken->start, NULL, 10);
        godot::UtilityFunctions::print("we got a number! %d\n", finalNode.bufferLength);
        currentToken++;
      }
      break;
      case CTOKEN_OR: {
        currentToken++;
        godot::UtilityFunctions::print("building or\n");
        finalFunc = binaryCommand(finalFunc, CTOKEN_OR);
        godot::UtilityFunctions::print("or control returned\n");
      }
      break;
      case CTOKEN_AND: {
        currentToken++;
        godot::UtilityFunctions::print("building and\n");
        finalFunc = binaryCommand(finalFunc, CTOKEN_AND);
      }
      break;
      default:
        break;
    }
  }

  return finalNode;
}

CommandFunction CommandCompiler::binaryCommand(CommandFunction currentFunc, CommandTokenType type){
  CommandNode nextStatement = compileOneNode();
  CommandFunction returnFunction = [currentFunc, nextStatement, type](int index, bool faceRight) -> bool {
    if(type == CTOKEN_OR){
      return (currentFunc(index, faceRight) || nextStatement.function(index, faceRight));
    } else if(type == CTOKEN_AND){
      return (currentFunc(index, faceRight) && nextStatement.function(index, faceRight));
    } else {
      return false;
    };
  };

  return returnFunction;
}
