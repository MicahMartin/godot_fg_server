#ifndef _CHARACTER_H
#define _CHARACTER_H

#include "VisualEffect.h"
#include "json.hpp"
#include "GameObject.h"
#include "StateDef.h"
#include "VirtualMachine.h"
#include "Script.h"
#include "VirtualController.h"
//#include "Animation.h"
#include "VisualEffect.h"
#include "Entity.h"

const int ENTITY_MAX = 3;
struct CharStateObj {
  int control,
  hitstun,
  blockstun,
  hitStop,
  pushTime,
  riskScaling,
  comboScale,
  comboDamage,
  pushBackVelocity,
  comebackCounter,
  hasAirAction,
  comboCounter,
  comboProration,
  cancelPointer,
  noGravityCounter,
  velocityX,
  velocityY,
  health,
  redHealth,
  redHealthCounter,
  gravityVal,
  hitPushTime,
  hitPushVelX,
  hitPushVelY,
  meter,
  comeback,
  flashCounter,
  auraID,
  timeInHitstun,
  hurtGravity,
  frameLastAttackConnected,
  currentState,
  positionX,
  positionY,
  throwInvul;

  bool inCorner,
  inHitStop,
  gravity,
  isDead,
  faceRight,
  inputFaceRight,
  isRed,
  isGreen,
  isLight,
  installMode,
  canThrow,
  auraActive;

  StateDefObj stateDefObj;
  VirtualControllerObj virtualControllerObj;
  EntityStateObj entityStates[1];
};

typedef enum {
  SS_NULL = 0,
  SS_IDLE = 1,
  SS_WALK_F,
  SS_WALK_B,
  SS_CROUCH,
  SS_JUMP_N = 5,
  SS_JUMP_F,
  SS_JUMP_B,
  SS_HURT,
  SS_HURT_RECOVERY,
  SS_AIR_HURT = 10,
  SS_AIR_HURT_RECOVERY,
  SS_KNOCKDOWN,
  SS_BLOCK_STAND,
  SS_BLOCK_CROUCH,
  SS_BLOWBACK_FALLING = 15,
  SS_AIR_BLOCK,
  SS_PRE_MATCH,
  SS_DEAD_STANDING,
  SS_DEAD_KNOCKDOWN,
  SS_THROW_TECH = 20,
  SS_AIR_THROW_TECH,
  SS_PUSH_BLOCK,
  SS_CROUCH_PUSH_BLOCK,
  SS_GROUNDBOUNCE_FLING,
  SS_GROUNDBOUNCE_IMPACT = 25,
  SS_AIR_PUSH_BLOCK,
  SS_JUMP_R,
  SS_AIR_TECH,
  SS_DEAD_FALLING,
  SS_FLOAT_HURT = 30,
  SS_FLOAT_HURT_RECOVERY,
  SS_FORWARD_THROW,
  SS_FORWARD_THROW_ATTEMPT,
  SS_FORWARD_THROW_SUCCESS,
  SS_GROUND_THROW_TECH = 35,
  SS_BACK_THROW,
  SS_BACK_THROW_ATTEMPT,
  SS_BACK_THROW_SUCCESS,
} SpecialState;

const int METER_ARRAY_MAX = 256;
class Character : public GameObject {
public:
  Character(std::pair<int, int> position, int playerNum);
  Character(std::pair<int, int> position);
  void init(const char* defPath);

  ~Character();

  void compileScript(const char* path, Script* script, const char* scriptTag);
  void loadStates(const char* path);
  void loadCustomStates(const char* path);
  void refresh();
  void changeState(int stateDefNum);
  void cancelState(int stateDefNum);
  void setCurrentState(int stateDefNum);

  void handleInput();
  void update();
  void draw();
  void drawEntities();
  void drawFX();

  CharStateObj saveState();
  void loadState(CharStateObj stateObj);
  int stateCount = 0;
  int vmCalls = 0;

  // position stuff
  std::pair<int,int> getPos();
  void setXPos(int x);
  void setYPos(int y);
  void setX(int x);
  void setY(int y);
  void updateFaceRight();
  void updatePosition();
  void updateCollisionBoxPositions();
  void updateCollisionBoxes();
  void activateVisFX(int visID);
  void countVmCalls(int frameCount);
  int getSoundChannel();
  int getAnimScale();
  bool hurtState(int state);
  bool airHurtState(int state);
  bool blockState(int state);
  bool pushBlockState(int state);
  bool checkBlock(int blockType);
  StateDef* getCurrentState();
  CollisionBox& getCollisionBox(int cbId);
  // Mix_Chunk* getSoundWithId(int id);

  void clearFlag(ObjFlag flag);
  void setFlag(ObjFlag flag);
  bool getFlag(ObjFlag flag);

  // getters for these guys
  int width = 10000;
  int maxHealth = 100;
  int maxMeter = 1000;
  int maxComeback = 1000;
  int hurtSoundMax = 3;

  int control = 1;
  int hitstun = 0;
  int blockstun = 0;
  int hitStop = 0;
  int pushTime = 0;
  int defenseValue = 100;
  int riskScaling = 1;
  int comboScale = 1;
  int pushBackVelocity = 0;
  int comboDamage = 1;
  int comebackCounter = 30;
  int hasAirAction = 0;
  int comboCounter = 0;
  int comboProration = 0;
  int cancelPointer = 0;
  int noGravityCounter = 0;
  long frameLastAttackConnected = 0;
  int gravityVal = 1;
  int velocityX = 0;
  int velocityY = 0;
  int velocityMinimumY = 0;
  int velocityMaximumY = 0;
  int velocityMinimumX = 0;
  int velocityMaximumX = 0;
  int health = 100;
  int redHealth = 100;
  int redHealthCounter = 0;
  int playerNum;
  bool faceRight;
  bool inputFaceRight;
  int hitPushTime = 0;
  int hitPushVelX = 0;
  int hitPushVelY = 0;
  int meter = 0;
  int meterArray[METER_ARRAY_MAX];
  int tensionGained = 0;
  int comeback = 750;
  int installCounter = 0;
  int currentHurtSoundID = 1;
  int soundChannel = 0;
  int flashCounter = 0;
  int auraID = 0;
  int throwInvul = 0;
  int hurtGravity = 1;
  int timeInHitstun = 1;
  float animScale = 4;
  double modelScale = 1.0;
  bool isDead = false;
  bool inCorner = false;
  bool inHitStop = false;
  bool gravity = true;
  bool isRed = false;
  bool isGreen = false;
  bool isLight = false;
  bool installMode = false;
  bool auraActive = false;
  bool canThrow = true;
  std::string charName = "";
  std::string modelName = "";
  std::string commandPath = "";
  std::pair<int, int> position;

  void _changeState(int stateNum);
  void _cancelState(int stateNum);
  void _velSetX(int ammount);
  void _negVelSetX(int ammount);
  void _velSetY(int ammount);
  void _moveForward(int ammount);
  void _moveBack(int ammount);
  void _moveUp(int ammount);
  void _moveDown(int ammount);
  void _setControl(int val);
  void _setCombo(int val);
  void _setHitStun(int val);
  void _setBlockstun(int val);
  void _setHitCancel(int val);
  void _setWhiffCancel(int val);
  void _setNoGravityCounter(int count);
  void _setGravity(int set);
  void _setAirAction(int set);
  void _setCounter(int val);
  void _activateEntity(int entityID);
  void _deactivateEntity(int entityID);
  void _snapToOpponent(int offset);
  void _addMeter(int input);
  void _setMeter(int input);
  void _subtractMeter(int input);
  void _setInstall(int input);
  void _resetAnim();

  int _getHitStun();
  int _getBlockStun();
  int _getAnimTime();
  int _getStateTime();
  int _getHitCancel();
  int _getWhiffCancel();
  int _getYPos();
  int _getVelY();
  int _getVelX();
  int _getStateNum();
  int _getControl();
  int _getCombo();
  int _getAirActions();
  int _getIsAlive();
  int _getInput(int input);
  int _wasPressed(int input);
  int _checkCommand(int commandIndex);
  int _getMeter(int meterIndex);
  int _getComebackMeter();
  int _getEntityStatus(int entityID);
  int _getInstall();

  VirtualController* virtualController;
  StateDef* currentState;
  Character* otherChar;
  Script inputScript;
  VirtualMachine virtualMachine;
  std::vector<uint8_t> inputByteCode;
  std::vector<Entity> entityList;

  // std::unordered_map<int, VisualEffect> hitSparks;
  // std::unordered_map<int, VisualEffect> guardSparks;
  VisualEffect hitSpark;
  VisualEffect guardSpark;
  // std::unordered_map<int, VisualEffect> visualEffects;
  // std::unordered_map<int, SoundObj> soundsEffects;
  // std::unordered_map<int, SoundObj> hurtSoundEffects;
  std::unordered_map<SpecialState, int> specialStateMap;
  int prevFrame = 0;
private:
  nlohmann::json stateJson;
  std::vector<StateDef> stateList;
};

#endif
