#include "Character.h"
#include "Entity.h"
#include "Util.h"
#include <_types/_uint64_t.h>
#include <fstream>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <sstream>
#include <iostream>



std::string p1InputHistory;
std::string p2InputHistory;

Character::Character(std::pair<int, int> _position, int _playerNum) {
  faceRight = playerNum == 1 ? true : false;
  health = 100;
  maxHealth = 100;
  velocityX = 0;
  velocityY = 0;
  position = _position;
  playerNum = _playerNum;
}

Character::Character(std::pair<int, int> _position) { 
  faceRight = playerNum == 1 ? true : false;
  health = 100;
  maxHealth = 100;
  position = _position;
}

void Character::init(const char* path){
  // stateObj.control = 0;
  control = 0;
  virtualMachine.character = this;
  stateList.reserve(256);
  entityList.reserve(12);
  loadStates(path);
  godot::UtilityFunctions::print("stateList size:", (uint64_t)stateList.size());
  // for (auto i : specialStateMap) {
  //   godot::UtilityFunctions::print("SPECIAL STATE MAP: ", i.first, "|" , i.second);
  // }
  changeState(specialStateMap[SS_PRE_MATCH]);
  hitSpark.setPlayLength(45);
}

CharStateObj Character::saveState(){
  CharStateObj stateObj;
  stateObj.control = control;
  stateObj.hitstun = hitstun;
  stateObj.blockstun = blockstun;
  stateObj.hitStop = hitStop;
  stateObj.pushTime = pushTime;
  stateObj.defenseValue = 1;
  stateObj.riskScaling = 1;
  stateObj.comboScale = 1;
  stateObj.pushBackVelocity = pushBackVelocity;
  stateObj.comboDamage = 1;
  stateObj.comebackCounter = comebackCounter;
  stateObj.hasAirAction = hasAirAction;
  stateObj.comboProration = comboProration;
  stateObj.comboCounter = comboCounter;
  stateObj.cancelPointer = cancelPointer;
  stateObj.noGravityCounter = noGravityCounter;
  stateObj.velocityX = velocityX;
  stateObj.velocityY = velocityY;
  stateObj.health = health;
  stateObj.redHealth = redHealth;
  stateObj.redHealthCounter = redHealthCounter;
  stateObj.gravityVal = gravityVal;
  stateObj.hitPushTime = hitPushTime;
  stateObj.hitPushVelX = hitPushVelX;
  stateObj.hitPushVelY = hitPushVelY;
  stateObj.meter = meter;
  stateObj.comeback = comeback;
  stateObj.flashCounter = flashCounter;
  stateObj.auraID = auraID;
  stateObj.timeInHitstun = timeInHitstun;
  stateObj.hurtGravity = hurtGravity;
  stateObj.frameLastAttackConnected = frameLastAttackConnected;
  stateObj.currentState = currentState->stateNum;
  stateObj.positionX = position.first;
  stateObj.positionY = position.second;
  stateObj.throwInvul = throwInvul;

  stateObj.inCorner = inCorner;
  stateObj.inHitStop = inHitStop;
  stateObj.gravity = gravity;
  stateObj.isDead = isDead;
  stateObj.faceRight = faceRight;
  stateObj.inputFaceRight = inputFaceRight;
  stateObj.isRed = isRed;
  stateObj.isGreen = isGreen;
  stateObj.isLight = isLight;
  stateObj.installMode = installMode;
  stateObj.canThrow = canThrow;
  stateObj.auraActive = auraActive;

  stateObj.stateDefObj = *currentState->saveState();
  for (int i = 0; i < entityList.size(); i++) {
    stateObj.entityStates[i] = entityList[i].saveState();
  }
  stateObj.virtualControllerObj = virtualController->saveState();
  return stateObj;
}

void Character::loadState(CharStateObj stateObj){
  control = stateObj.control;
  hitstun = stateObj.hitstun;
  blockstun = stateObj.blockstun;
  hitStop = stateObj.hitStop;
  pushTime = stateObj.pushTime;
  pushBackVelocity = stateObj.pushBackVelocity;
  comebackCounter = stateObj.comebackCounter;
  hasAirAction = stateObj.hasAirAction;
  comboCounter = stateObj.comboCounter;
  cancelState(stateObj.cancelPointer);
  noGravityCounter = stateObj.noGravityCounter;
  velocityX = stateObj.velocityX;
  velocityY = stateObj.velocityY;
  health = stateObj.health;
  redHealth = stateObj.redHealth;
  redHealthCounter = stateObj.redHealthCounter;
  gravityVal = stateObj.gravityVal;
  hitPushTime = stateObj.hitPushTime;
  hitPushVelX = stateObj.hitPushVelX;
  hitPushVelY = stateObj.hitPushVelY;
  meter = stateObj.meter;
  comeback = stateObj.comeback;
  flashCounter = stateObj.flashCounter;
  auraID = stateObj.auraID;
  timeInHitstun = stateObj.timeInHitstun;
  hurtGravity = stateObj.hurtGravity;
  frameLastAttackConnected = stateObj.frameLastAttackConnected;
  setCurrentState(stateObj.currentState);
  position.first = stateObj.positionX;
  position.second = stateObj.positionY;
  throwInvul = stateObj.throwInvul;

  inCorner = stateObj.inCorner;
  inHitStop = stateObj.inHitStop;
  gravity = stateObj.gravity;
  isDead = stateObj.isDead;
  faceRight = stateObj.faceRight;
  inputFaceRight = stateObj.inputFaceRight;
  isRed = stateObj.isRed;
  isGreen = stateObj.isGreen;
  isLight = stateObj.isLight;
  installMode = stateObj.installMode;
  canThrow = stateObj.canThrow;
  auraActive = stateObj.auraActive;
  
  currentState->loadState(stateObj.stateDefObj);
  for (int i = 0; i < entityList.size(); i++) {
    entityList[i].loadState(stateObj.entityStates[i]);
  }
  virtualController->loadState(stateObj.virtualControllerObj);
}

void Character::refresh(){
  changeState(specialStateMap[SS_PRE_MATCH]);
  health = 100;
  redHealth = 100;
  control = 1;
  hitstun = 0;
  blockstun = 0;
  hitStop = 0;
  pushTime = 0;
  pushBackVelocity = 0;
  hasAirAction = 0;
  comboCounter = 0;
  cancelPointer = 0;
  noGravityCounter = 0;
  frameLastAttackConnected = 0;
  inCorner = false;
  inHitStop = false;
  gravity = true;
  isDead = false;
  velocityX = 0;
  velocityY = 0;
  currentHurtSoundID = 1;
}

void Character::changeState(int stateDefNum){
  // for (auto &i : visualEffects) {
  //   if (i.second.getAura()) {
  //     i.second.setActive(false);
  //   }
  // }
  auraActive = false;
  cancelPointer = 0;
  if(stateDefNum >= 6000){
    int theNum = stateDefNum - 6000;
    int customStateNum = stateCount + theNum;

    currentState = &stateList.at(customStateNum-1);
  } else if (stateDefNum >= 5000) {
    int theNum = stateDefNum - 5000;
    SpecialState theState = (SpecialState)theNum;
    int specialStateNum = specialStateMap[theState];
    // printf("the num:%d, the specialStateNum %d\n", stateDefNum, specialStateNum);
    currentState = &stateList.at(specialStateNum-1);
  } else {
    currentState = &stateList.at(stateDefNum-1);
  }

  if(!currentState->checkFlag(NO_TURN_ON_ENTER)){
    updateFaceRight();
  }
  currentState->enter();
  updateCollisionBoxPositions();
  updateCollisionBoxes();
};

void Character::setCurrentState(int stateDefNum){
  // godot::UtilityFunctions::print("setting state from save:", stateDefNum);
  if(stateDefNum >= 6000){
    int theNum = stateDefNum - 6000;
    int customStateNum = stateCount + theNum;
    godot::UtilityFunctions::print("loading custom state");
    // printf("the num:%d, the customStateNum%d\n", stateDefNum, customStateNum);
    currentState = &stateList.at(customStateNum-1);
  } else if (stateDefNum >= 5000) {
    int theNum = stateDefNum - 5000;
    SpecialState theState = (SpecialState)theNum;
    int specialStateNum = specialStateMap[theState];
    // printf("the num:%d, the specialStateNum %d\n", stateDefNum, specialStateNum);
    currentState = &stateList.at(specialStateNum-1);
  } else {
    // printf("the the stateNum:%d\n", stateDefNum);
    currentState = &stateList.at(stateDefNum-1);
  }
}

void Character::cancelState(int stateDefNum){
  // printf("oh yeah cancel that bitch %d\n", stateDefNum);
  if (stateDefNum >= 6000) {
    int theNum = stateDefNum - 6000;
    int customState = stateCount + theNum;
    // printf("the num:%d, the customState%d\n", theNum, customState);
    cancelPointer = customState;
  } else if (stateDefNum >= 5000) {
    int theNum = stateDefNum - 5000;
    SpecialState theState = (SpecialState)theNum;
    int specialStateNum = specialStateMap[theState];
    cancelPointer = specialStateNum;
  } else {
    cancelPointer = stateDefNum;
  }
};

void Character::loadStates(const char* path){
  // printf("%d Loading states\n", playerNum);
  std::ifstream configFile(path);
  configFile >> stateJson;
  // compile inputs
  if (stateJson.count("model_name")) {
    modelName = stateJson.at("model_name");
  }
  if (stateJson.count("model_scale")) {
    std::string animScaleStr = stateJson.at("model_scale");
    modelScale = std::stod(animScaleStr);
    godot::UtilityFunctions::print("The model scale is", modelScale);
  }
  if (stateJson.count("gravity")) {
    std::string grav = stateJson.at("gravity");
    godot::UtilityFunctions::print("the grav: ", grav.c_str());
    gravityVal = std::stod(grav);
  }
  if (stateJson.count("velocityMaximumX")) {
  }
  if (stateJson.count("velocityMinimumX")) {
  }
  if (stateJson.count("velocityMaximumY")) {
  }
  if (stateJson.count("velocityMinimumY")) {
    velocityMinimumY = stateJson.at("velocityMinimumY");
  }

  std::string str = stateJson.at("command_script").get<std::string>();
  const char* ptr = str.c_str();
  if(!virtualMachine.compiler.compile(ptr, &inputScript, "input script")){
    inputScript.disassembleScript("input command script");
    godot::UtilityFunctions::print("inputscript failed to compile");
    throw std::runtime_error("inputScript failed to compile");
  }

  // load states
  printf("loading states\n");
  for(auto i : stateJson.at("states").items()){
    auto& newStateDef = stateList.emplace_back();
    newStateDef.owner = this;
    newStateDef.charName = charName;
    newStateDef.init(i.value(), &virtualMachine, animScale);
  }

  printf("loading entities\n");
  entityList.reserve(12);
  for(auto i : stateJson.at("entities").items()){
    if (i.value().count("defPath")) {
      std::string defString = i.value().at("defPath");
      printf("the defPath %s\n", defString.c_str());
      auto& newEntity = entityList.emplace_back(this, i.value().at("entityID"), defString.c_str());
      newEntity.init();
      printf("done loading entities\n");
    }
  }

  for(auto i : stateJson.at("animation_assets").items()){
    int visualID = i.value().at("assetID");
    // visualEffects.emplace(visualID, VisualEffect{}).first->second.anim.loadAnimEvents(i.value().at("animation"));
    // visualEffects.at(visualID).setPlayLength(visualEffects.at(visualID).anim.animationTime);
    if (i.value().count("aura")) {
      // visualEffects.at(visualID).setAura(i.value().at("aura"));
    }
    if (i.value().count("charEffect")) {
      // visualEffects.at(visualID).charEffect = (i.value().at("charEffect"));
    }
    printf("loaded visual effect # %d\n", visualID);
  }

  for(auto i : stateJson.at("hit_sparks").items()){
    int visualID = i.value().at("assetID");
    // hitSparks.emplace(visualID, VisualEffect{}).first->second.anim.loadAnimEvents(i.value().at("animation"));
    // hitSparks.at(visualID).setPlayLength(hitSparks.at(visualID).anim.animationTime);
    printf("loaded hitspark effect #%d\n", visualID);
  }

  for(auto i : stateJson.at("guard_sparks").items()){
    int visualID = i.value().at("assetID");
    // guardSparks.emplace(visualID, VisualEffect{}).first->second.anim.loadAnimEvents(i.value().at("animation"));
    // guardSparks.at(visualID).setPlayLength(guardSparks.at(visualID).anim.animationTime);
    printf("loaded guardpark effect #%d\n", visualID);
  }

  for(auto i : stateJson.at("audio_assets").items()){
    int soundID = i.value().at("assetID");

    std::string path(i.value().at("file").get<std::string>());
    const char* pathPointer = path.c_str();

    // Mix_Chunk* soundEffectPointer = Mix_LoadWAV(pathPointer);
    // soundsEffects.emplace(soundID, SoundObj{soundEffectPointer, false, 0});
  }
  for(auto i : stateJson.at("hurt_sounds").items()){
    int hurtSoundID = i.value().at("assetID");

    std::string path(i.value().at("file").get<std::string>());
    const char* pathPointer = path.c_str();

    // Mix_Chunk* hurtSoundPointer = Mix_LoadWAV(pathPointer);
    // hurtSoundEffects.emplace(hurtSoundID, SoundObj{hurtSoundPointer, false, 0, soundChannel + 1});
  }
  // loadSpecialStates
  {
    auto i = stateJson.at("special_state_nums");
    specialStateMap[SS_IDLE] = i.at("SS_IDLE");
    specialStateMap[SS_WALK_F] = i.at("SS_WALK_F");
    specialStateMap[SS_WALK_B] = i.at("SS_WALK_B");
    specialStateMap[SS_CROUCH] = i.at("SS_CROUCH");
    specialStateMap[SS_JUMP_N] = i.at("SS_JUMP_N");
    specialStateMap[SS_JUMP_F] = i.at("SS_JUMP_F");
    specialStateMap[SS_JUMP_B] = i.at("SS_JUMP_B");
    specialStateMap[SS_HURT] = i.at("SS_HURT");
    specialStateMap[SS_HURT_RECOVERY] = i.at("SS_HURT_RECOVERY");
    specialStateMap[SS_AIR_HURT] = i.at("SS_AIR_HURT");
    specialStateMap[SS_AIR_HURT_RECOVERY] = i.at("SS_AIR_HURT_RECOVERY");
    specialStateMap[SS_KNOCKDOWN] = i.at("SS_KNOCKDOWN");
    specialStateMap[SS_BLOCK_STAND] = i.at("SS_BLOCK_STAND");
    specialStateMap[SS_BLOCK_CROUCH] = i.at("SS_BLOCK_CROUCH");
    specialStateMap[SS_BLOWBACK_FALLING] = i.at("SS_BLOWBACK_FALLING");
    specialStateMap[SS_AIR_BLOCK] = i.at("SS_AIR_BLOCK");
    specialStateMap[SS_PRE_MATCH] = i.at("SS_PRE_MATCH");
    specialStateMap[SS_DEAD_STANDING] = i.at("SS_DEAD_STANDING");
    specialStateMap[SS_DEAD_KNOCKDOWN] = i.at("SS_DEAD_KNOCKDOWN");
    specialStateMap[SS_KNOCKDOWN] = i.at("SS_KNOCKDOWN");
    specialStateMap[SS_THROW_TECH] = i.at("SS_THROW_TECH");
    specialStateMap[SS_AIR_THROW_TECH] = i.at("SS_AIR_THROW_TECH");
    specialStateMap[SS_PUSH_BLOCK] = i.at("SS_PUSH_BLOCK");
    specialStateMap[SS_CROUCH_PUSH_BLOCK] = i.at("SS_CROUCH_PUSH_BLOCK");
    specialStateMap[SS_GROUNDBOUNCE_FLING] = i.at("SS_GROUNDBOUNCE_FLING");
    specialStateMap[SS_GROUNDBOUNCE_IMPACT] = i.at("SS_GROUNDBOUNCE_IMPACT");
    specialStateMap[SS_JUMP_R] = i.at("SS_JUMP_R");
    specialStateMap[SS_AIR_TECH] = i.at("SS_AIR_TECH");
    specialStateMap[SS_DEAD_FALLING] = i.at("SS_DEAD_FALLING");
    specialStateMap[SS_FLOAT_HURT] = i.at("SS_FLOAT_HURT");
    specialStateMap[SS_FLOAT_HURT_RECOVERY] = i.at("SS_FLOAT_HURT_RECOVERY");
    specialStateMap[SS_FORWARD_THROW] = i.at("SS_FORWARD_THROW");
    specialStateMap[SS_FORWARD_THROW_ATTEMPT] = i.at("SS_FORWARD_THROW_ATTEMPT");
    specialStateMap[SS_FORWARD_THROW_SUCCESS] = i.at("SS_FORWARD_THROW_SUCCESS");
    specialStateMap[SS_GROUND_THROW_TECH] = i.at("SS_GROUND_THROW_TECH");
    specialStateMap[SS_BACK_THROW] = i.at("SS_BACK_THROW");
    specialStateMap[SS_BACK_THROW_ATTEMPT] = i.at("SS_BACK_THROW_ATTEMPT");
    specialStateMap[SS_BACK_THROW_SUCCESS] = i.at("SS_BACK_THROW_SUCCESS");
  }


  configFile.close();
  stateCount = stateList.size();
}

void Character::loadCustomStates(const char* path){
  stateJson.clear();
  std::ifstream configFile(path);
  configFile >> stateJson;
  // load states
  printf("loading custom states\n");
  godot::UtilityFunctions::print("loading custom states");
  int counter = 0;

  for(auto i : stateJson.at("custom_states").items()){
    StateDef* newStateDef = &stateList.emplace_back();
    newStateDef->owner = this;
    newStateDef->charName = charName;
    newStateDef->init(i.value(), &virtualMachine, animScale);
    ++counter;
  }
  printf("custom state count value:%d\n", counter);
  godot::UtilityFunctions::print("custom state count value:", counter);

  configFile.close();
  stateJson.clear();
}

Character::~Character(){};

void Character::handleInput(){ 
  // hmm? can we call 3 scripts per frame?
  // how many times can we change state per frame?
  if(cancelPointer != 0 && !inHitStop){
    changeState(cancelPointer);
  }

  if(control){
    // vmCalls++;
    virtualMachine.execute(&inputScript);
  }
};

void Character::update(){ 
  if (installMode) {
    if (installCounter++ == 1) {
      installCounter = 0;
      comeback -= 3;
    }
  }
  if(pushTime > 0) {
    pushTime--;
    if(pushTime == 0){
      pushBackVelocity = 0;
    }
  }
  if(hitPushTime > 0) {
    hitPushTime--;
    if(hitPushTime == 0){
      hitPushVelX = 0;
    }
  }

  if (hitstun > 0) {
    hitstun--;
  }

  if (blockstun > 0) {
    blockstun--;
  }

  if (comebackCounter-- == 0) {
    comebackCounter = 30;
    if (comeback++ >= maxComeback) {
      comeback = maxComeback;
    }
  }


  if (currentState->visualEffectMap.count(currentState->stateTime)) {
    int visualID = currentState->visualEffectMap.at(currentState->stateTime);
    // VisualEffect& visFX = visualEffects.at(visualID);
    // printf("found visFX for frame %d, the playLEngth %d\n", currentState->stateTime, visFX.getPlayLength());
    // visFX.reset(position.first, position.second);
    // visFX.setActive(true);
  }
  for (auto i : currentState->visualEffectMap) {
    // VisualEffect& visFX = visualEffects.at(i.second);
    // printf("found visFX for frame %d, the playLEngth %d\n", currentState->stateTime, visFX.getPlayLength());
    // if (visFX.getAura()) {
    //   visFX.setX(position.first);
    //   visFX.setY(position.second);
    // }
  }
  if (currentState->soundIndexMap[currentState->stateTime].size() > 0) {
    for (int soundID : currentState->soundIndexMap[currentState->stateTime]) {
      // soundsEffects.at(soundID).active = true;
      // soundsEffects.at(soundID).channel = soundChannel;
    }
  }

  if (isRed) {
    isRed = false;
  }
  if (isGreen) {
    isGreen = false;
  }
  if (isLight) {
    isLight = false;
  }
  if (installMode) {
    if(flashCounter++ < 8){
      isRed = true;
    } else if(flashCounter < 16){
      isRed = false;
    }
    if(flashCounter == 16){
      flashCounter = 0;
    }
  }

  // godot::UtilityFunctions::print("stateTime:", currentState->stateTime);
  // vmCalls++;
  //TODO: Should i be running the vm 3 times a frame??
  currentState->update();

  updatePosition();
  updateCollisionBoxPositions();
  updateCollisionBoxes();
};

void Character::updateFaceRight(){
  if (position.first == otherChar->getPos().first) {
  } else {
    if(position.first < otherChar->getPos().first){
      faceRight = true;
      inputFaceRight = true;
    } else {
      faceRight = false;
      inputFaceRight = false;
    }
  }
};

void Character::activateVisFX(int visID){
  // VisualEffect& visFX = visualEffects.at(visID);
  // visFX.reset(position.first, position.second);
  // visFX.setActive(true);
}

void Character::updatePosition() {
  // _negVelSetX(pushBackVelocity);
  int velX = velocityX;
  int stateNum = currentState->stateNum;
  bool inHurtState = stateNum == specialStateMap[SS_HURT]
    || stateNum == specialStateMap[SS_AIR_HURT]
    || stateNum == specialStateMap[SS_BLOWBACK_FALLING]
    || stateNum == specialStateMap[SS_GROUNDBOUNCE_FLING]
    || stateNum == specialStateMap[SS_GROUNDBOUNCE_IMPACT];
  if (hitstun > 0 || inHurtState) {
    velX = velocityX - hitPushVelX;
  } else {
    velX = velocityX - pushBackVelocity;
  }
  position.first += velX;
  position.second += velocityY;

  if(noGravityCounter > 0){
    gravity = false;
    if(--noGravityCounter <= 0){
      noGravityCounter = 0;
      gravity = true;
    }
  }

  if(position.second > 0 && gravity){
    velocityY -= gravityVal;
    if ((velocityMinimumY != 0) && velocityY < velocityMinimumY) {
      velocityY = velocityMinimumY;
    }
  }

  if(position.second < 0){
    position.second = 0;
    velocityY = 0;
  }
}

void Character::updateCollisionBoxPositions(){
  for (auto& cbId : currentState->pushBoxIds) {
    getCollisionBox(cbId).positionX = position.first - (getCollisionBox(cbId).width/2);
    getCollisionBox(cbId).positionY = position.second + getCollisionBox(cbId).offsetY;
  }

  for (auto& cbId : currentState->hurtBoxIds) {
    getCollisionBox(cbId).positionX = position.first + (faceRight ? getCollisionBox(cbId).offsetX : - (getCollisionBox(cbId).offsetX + getCollisionBox(cbId).width));
    getCollisionBox(cbId).positionY = position.second + getCollisionBox(cbId).offsetY;
  }

  for (auto& cbId : currentState->hitBoxIds) {
    getCollisionBox(cbId).positionX = position.first + (faceRight ? getCollisionBox(cbId).offsetX : - (getCollisionBox(cbId).offsetX + getCollisionBox(cbId).width));
    getCollisionBox(cbId).positionY = position.second + getCollisionBox(cbId).offsetY;
  }

  for (auto& cbId : currentState->throwHitBoxIds) {
    getCollisionBox(cbId).positionX = position.first + (faceRight ? getCollisionBox(cbId).offsetX : - (getCollisionBox(cbId).offsetX + getCollisionBox(cbId).width));
    getCollisionBox(cbId).positionY = position.second + getCollisionBox(cbId).offsetY;
  }

  for (auto& cbId : currentState->throwHurtBoxIds) {
    getCollisionBox(cbId).positionX = position.first + (faceRight ? getCollisionBox(cbId).offsetX : - (getCollisionBox(cbId).offsetX + getCollisionBox(cbId).width));
    getCollisionBox(cbId).positionY = position.second + getCollisionBox(cbId).offsetY;
  }

  for (auto& cbId : currentState->proximityBoxIds) {
    getCollisionBox(cbId).positionX = position.first + (faceRight ? getCollisionBox(cbId).offsetX : - (getCollisionBox(cbId).offsetX + getCollisionBox(cbId).width));
    getCollisionBox(cbId).positionY = position.second + getCollisionBox(cbId).offsetY;
  }
  for (auto& cbId : currentState->projectileBoxIds) {
    getCollisionBox(cbId).positionX = position.first + (faceRight ? getCollisionBox(cbId).offsetX : - (getCollisionBox(cbId).offsetX + getCollisionBox(cbId).width));
    getCollisionBox(cbId).positionY = position.second + getCollisionBox(cbId).offsetY;
  }
}

void Character::updateCollisionBoxes(){
  // TODO: abstract into updateCollisionBoxPos function
  int stateTime = currentState->stateTime + 1;
  for (auto& cbId : currentState->pushBoxIds) {
    if (stateTime < getCollisionBox(cbId).start) {
      getCollisionBox(cbId).disabled = true;
    }
    if ((getCollisionBox(cbId).end == -1 && (stateTime > getCollisionBox(cbId).start)) || stateTime == getCollisionBox(cbId).start) {
      getCollisionBox(cbId).disabled = false;
    }
    if (stateTime == getCollisionBox(cbId).end) {
      getCollisionBox(cbId).disabled = true;
    }
  }

  for (auto& cbId : currentState->hurtBoxIds) {
    if (stateTime < getCollisionBox(cbId).start) {
      getCollisionBox(cbId).disabled = true;
    }
    if ((getCollisionBox(cbId).end == -1 && (stateTime > getCollisionBox(cbId).start)) || stateTime == getCollisionBox(cbId).start) {
      getCollisionBox(cbId).disabled = false;
    }
    if (stateTime == getCollisionBox(cbId).end) {
      getCollisionBox(cbId).disabled = true;
    }
  }

  for (auto& cbId : currentState->throwHurtBoxIds) {
    if (stateTime < getCollisionBox(cbId).start) {
      getCollisionBox(cbId).disabled = true;
    }
    if (getCollisionBox(cbId).end == -1 || stateTime == getCollisionBox(cbId).start) {
      getCollisionBox(cbId).disabled = false;
    }
    if (stateTime == getCollisionBox(cbId).end) {
      getCollisionBox(cbId).disabled = true;
    }
  }

  for (auto& cbId : currentState->hitBoxIds) {
    if (stateTime < getCollisionBox(cbId).start) {
      getCollisionBox(cbId).disabled = true;
    }
    if (stateTime == getCollisionBox(cbId).start) {
      getCollisionBox(cbId).disabled = false;
    }
    if (stateTime == getCollisionBox(cbId).end) {
      getCollisionBox(cbId).disabled = true;
    }
  }

  for (auto& cbId : currentState->throwHitBoxIds) {
    if (stateTime < getCollisionBox(cbId).start) {
      getCollisionBox(cbId).disabled = true;
    }
    if (stateTime == getCollisionBox(cbId).start) {
      getCollisionBox(cbId).disabled = false;
    }
    if (stateTime == getCollisionBox(cbId).end) {
      getCollisionBox(cbId).disabled = true;
    }
  }

  for (auto& cbId : currentState->proximityBoxIds) {
    if (stateTime < getCollisionBox(cbId).start) {
      getCollisionBox(cbId).disabled = true;
    }
    if (stateTime == getCollisionBox(cbId).start) {
      getCollisionBox(cbId).disabled = false;
    }
    if (stateTime == getCollisionBox(cbId).end) {
      getCollisionBox(cbId).disabled = true;
    }
  }
  for (auto& cbId : currentState->projectileBoxIds) {
    if (stateTime < getCollisionBox(cbId).start) {
      getCollisionBox(cbId).disabled = true;
    }
    if (stateTime == getCollisionBox(cbId).start) {
      getCollisionBox(cbId).disabled = false;
    }
    if (stateTime == getCollisionBox(cbId).end) {
      getCollisionBox(cbId).disabled = true;
    }
  }
}

std::pair<int,int> Character::getPos(){
  return position;
};

StateDef* Character::getCurrentState(){
  return currentState;
};

CollisionBox& Character::getCollisionBox(int cbId){
  return currentState->collisionBoxes[cbId];
};

int Character::getSoundChannel(){
  return soundChannel;
};

int Character::getAnimScale(){
  return animScale;
};

void Character::setXPos(int x){
  position.first = x;
};

void Character::setYPos(int y){
  position.second = y;
};

void Character::setX(int x){
  position.first += x;
};

void Character::setY(int y){
  position.second += y;
};

bool Character::hurtState(int state){
  return (
      state == specialStateMap[SS_HURT]
      || state == specialStateMap[SS_HURT_RECOVERY]
      || state == specialStateMap[SS_AIR_HURT]
      || state == specialStateMap[SS_AIR_HURT_RECOVERY]
      || state == specialStateMap[SS_BLOWBACK_FALLING]
      || state == specialStateMap[SS_DEAD_STANDING]
      || state == specialStateMap[SS_DEAD_KNOCKDOWN]
      || state == specialStateMap[SS_GROUNDBOUNCE_FLING]
      || state == specialStateMap[SS_GROUNDBOUNCE_IMPACT]);
}

bool Character::airHurtState(int state){
  return (
    state == specialStateMap[SS_AIR_HURT]
    || state == specialStateMap[SS_AIR_HURT_RECOVERY]
    || state == specialStateMap[SS_BLOWBACK_FALLING]
    || state == specialStateMap[SS_DEAD_STANDING]
    || state == specialStateMap[SS_DEAD_KNOCKDOWN]
    || state == specialStateMap[SS_GROUNDBOUNCE_FLING]
    || state == specialStateMap[SS_GROUNDBOUNCE_IMPACT]
    || state == specialStateMap[SS_FLOAT_HURT]
    || state == specialStateMap[SS_FLOAT_HURT_RECOVERY]
  );
}

bool Character::blockState(int state){
  return ( 
    state == specialStateMap[SS_BLOCK_STAND]
    || state == specialStateMap[SS_BLOCK_CROUCH]
    || state == specialStateMap[SS_AIR_BLOCK]
    || state == specialStateMap[SS_PUSH_BLOCK]
    || state == specialStateMap[SS_CROUCH_PUSH_BLOCK]
    || state == specialStateMap[SS_AIR_PUSH_BLOCK]
  );
}  

bool Character::pushBlockState(int state){
  return (
    state == specialStateMap[SS_PUSH_BLOCK]
    || state == specialStateMap[SS_CROUCH_PUSH_BLOCK]
    || state == specialStateMap[SS_AIR_PUSH_BLOCK]
  );
}

bool Character::checkBlock(int blockType){
  bool isHoldingDownBack = _getInput(1);
  bool isHoldingBack = _getInput(4);
  bool upBackinScrub = _getInput(7);
  // I know, enum
  if (_getYPos() > 0) {
    if (isHoldingBack || isHoldingDownBack || upBackinScrub) {
      return true;
    }
  }
  switch (blockType) {
    // mid
    case 1:
      if (isHoldingDownBack || isHoldingBack)
        return true;
      break;
      // low
    case 2:
      if (isHoldingDownBack)
        return true;
      break;
    case 3:
      // high
      if (isHoldingBack)
        return true;
      break;
    default:
      return true;
  }

  return false;
}  

void Character::setFlag(ObjFlag flag){

  flags |= flag;
}

void Character::clearFlag(ObjFlag flag){

  flags &= (~flag);
}

bool Character::getFlag(ObjFlag flag){

  return (( flags & flag ) == flag);
}


void Character::_changeState(int  stateNum){
  changeState(stateNum);
}

void Character::_cancelState(int  stateNum){
  cancelState(stateNum);
}

void Character::_velSetX(int ammount){
 velocityX = faceRight ? ammount : -ammount;
}

void Character::_negVelSetX(int ammount){
 velocityX = faceRight ? -ammount : ammount;
}

void Character::_velSetY(int ammount){
  velocityY = ammount;
}

void Character::_moveForward(int ammount){
  velocityX += faceRight ? ammount : -ammount;
}

void Character::_moveBack(int ammount){
  faceRight ? setX(-ammount) : setX(ammount);
}

void Character::_moveUp(int ammount){
  setY(ammount);
}

void Character::_moveDown(int ammount){
  setY(-ammount);
}

void Character::_setControl(int val){
  control = val;
}

void Character::_setCombo(int val){
  comboCounter = val;
}

void Character::_setGravity(int on){
  gravity = on;
}

void Character::_setNoGravityCounter(int count){
  noGravityCounter += count;
}

void Character::_resetAnim(){
  currentState->resetAnim();
}

void Character::_activateEntity(int entityID){
  entityList[entityID - 1].activateEntity();
}

void Character::_deactivateEntity(int entityID){
  printf("deactivating entity:%d \n", entityID);
  entityList[entityID - 1].deactivateEntity();
}

void Character::_snapToOpponent(int offset){
  auto opponentPos = otherChar->getPos();
  bool opponentFaceRight = otherChar->faceRight;

  position.first = opponentFaceRight ? (opponentPos.first + offset) : (opponentPos.first - offset);
  position.second = opponentPos.second;
  printf("snapping to opponentPos:%d:%d, newPos:%d:%d\n", opponentPos.first, opponentPos.second, position.first, position.second);
}


int Character::_getAnimTime(){
  // return currentState->anim.timeRemaining();
  return 0;
}

int Character::_getYPos(){
  int yPos = abs(getPos().second);
  return yPos;
}

int Character::_getStateTime(){
  return currentState->stateTime;
}

int Character::_getInput(int input){
  // godot::UtilityFunctions::print("In get Input");
  Input inputType = VirtualController::inputMap[input](inputFaceRight);
  return virtualController->isPressed(inputType) ? 1 : 0;
}

int Character::_getStateNum(){
  return currentState->stateNum;
}

int Character::_getControl(){
  return control;
}

int Character::_getAirActions(){
  return hasAirAction;
}

void Character::_setHitStun(int operand){
  hitstun = operand;
}

void Character::_setHitCancel(int val){
  currentState->canHitCancel = val;
}

void Character::_setWhiffCancel(int val){
  currentState->canWhiffCancel = val;
}

void Character::_setAirAction(int operand){
  if(operand < 0){
    hasAirAction += operand;
  } else {
    hasAirAction = operand;
  }
}

void Character::_setCounter(int operand){
  currentState->counterHitFlag = operand;
}

int Character::_getCombo(){
  return comboCounter;
}

int Character::_wasPressed(int input){
  return virtualController->wasPressedBuffer(VirtualController::inputMap[input](inputFaceRight)) ? 1 : 0;
}

int Character::_getHitStun(){
  return hitstun;
}

int Character::_getHitCancel(){
  return currentState->canHitCancel;
}

int Character::_getWhiffCancel(){
  return currentState->canWhiffCancel;
}

int Character::_getBlockStun(){
  return blockstun;
}

int Character::_getIsAlive(){
  return !isDead;
}

int Character::_getMeter(int meterIndex){
  return meterArray[meterIndex];
}

int Character::_getComebackMeter(){
  return comeback;
}

void Character::_addMeter(int i){
  std::string meterString = std::to_string(i);
  int meterIndex = meterString.front();
  int meterValue = std::stoi(meterString.substr(1));
  meterArray[meterIndex] += meterValue;
}

void Character::_setMeter(int i){
  std::string meterString = std::to_string(i);
  int meterIndex = meterString.front();
  int meterValue = std::stoi(meterString.substr(1));
  meterArray[meterIndex] = meterValue;
}

void Character::_subtractMeter(int i){
  std::string meterString = std::to_string(i);
  int meterIndex = meterString.front();
  int meterValue = std::stoi(meterString.substr(1));
  meterArray[meterIndex] -= meterValue;
}

int Character::_checkCommand(int commandIndex){
  return virtualController->checkCommand(commandIndex, inputFaceRight);
}

void Character::_setBlockstun(int input){
  blockstun = input;
};

void Character::_setInstall(int input){
  printf("setting installMode to %d\n", input);
  installMode = input;
};

int Character::_getEntityStatus(int entityID){
  printf("is the entity active? %d, %d\n", entityID, entityList[entityID - 1].active);
  return entityList.at(entityID - 1).active;
};

int Character::_getInstall(){
  return installMode;
};

int Character::_getVelY(){
  return velocityY;
};

int Character::_getVelX(){
  return faceRight ? velocityX : velocityX * -1;
};
