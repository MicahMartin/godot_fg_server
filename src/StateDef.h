#ifndef _StateDef_h
#define _StateDef_h

#include <vector>
#include "json.hpp"
#include "GameObject.h"
#include "VirtualController.h"
// #include "graphics/Animation.h"
#include "CollisionBox.h"
#include "Script.h"
#include "VirtualMachine.h"


typedef std::vector<CollisionBox> CollisionBoxList;

enum FlagBit {
  NO_TURN = 0x01,
  NO_TURN_ON_ENTER  = 0x02,
  TECHABLE = 0x04,
  SUPER_ATTACK = 0x08,
};


struct SoundObj {
  // Mix_Chunk* sound;
  bool active = false;
  int soundID = 0;
  int channel = -1;
};

const int HITBOX_GROUP_MAX = 8;
const int COLLISION_BOX_MAX = 16;

struct StateDefObj {
  int stateTime;
  int animTime;
  int freezeFrame;
  int freezeLength;
  bool hitboxesDisabled;
  bool canWhiffCancel;
  bool canHitCancel;
  bool counterHitFlag;

  bool hitboxGroupDisabled[HITBOX_GROUP_MAX];
  CollisionBoxState collisionBoxStates[COLLISION_BOX_MAX];
};
class GameObject;
class StateDef {
public:
  StateDef();
  ~StateDef();

  // load shit
  void init(nlohmann::json::value_type json, VirtualMachine* charVm, float _animScale);
  void loadFlags(nlohmann::json::value_type json);
  void loadAnimation(nlohmann::json json);
  void loadCollisionBoxes(nlohmann::json json);
  void loadVisualEffects(nlohmann::json json);

  bool checkFlag(FlagBit bit);

  void enter();
  void handleCancels();
  void update();
  void draw(std::pair<int,int> position, bool faceRight, bool inHitStop);
  void drawCollisionBoxes();

  StateDefObj* saveState();
  void loadState(StateDefObj stateObj);

  void resetAnim();


  GameObject* owner;
  VirtualMachine* charVm;
  Script updateScript;
  Script cancelScript;

  // TODO: Polymorph or atleast use a union
  std::vector<int> pushBoxIds;
  std::vector<int> hurtBoxIds;
  std::vector<int> hitBoxIds;
  std::vector<int> throwHitBoxIds;
  std::vector<int> throwHurtBoxIds;
  std::vector<int> proximityBoxIds;
  std::vector<int> projectileBoxIds;
  std::vector<int> triggerBoxIds;
  CollisionBox collisionBoxes[COLLISION_BOX_MAX];
  int collisionBoxCount = 0;

  std::unordered_map<int, std::vector<int>> soundIndexMap;
  std::unordered_map<int, int> visualEffectMap; // <startFrame, visualID>
  // TODO: Methods to talk to anim so this stuff can stay private
  int stateNum;
  int techState;

  int stateTime = 0;
  int animTime = 0;
  int freezeFrame = 0;
  int freezeLength = 0;
  float animScale = 1;
  bool hitboxesDisabled = false;

  bool canWhiffCancel = false;
  bool canHitCancel = false;
  bool counterHitFlag = false;
  bool loopAnimation = false;
  std::unordered_map<int, bool> hitboxGroupDisabled;
  std::string charName = "";
  std::string animationPath = "";

  StateDefObj stateObj;
private:
  static std::map<std::string, FlagBit> flagMap;
  uint8_t flagByte = 0;
};
#endif
