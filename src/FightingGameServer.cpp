#include <chrono>
#include <cstdlib>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/Input.hpp>
#include <bitset>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include "FightingGameServer.h"
#include "ggponet.h"
#include "Util.h"

#ifdef _WIN32
  #include <Windows.h>
#else
  #include <unistd.h>
#endif

// #define SYNC_TEST

GGPOSession* ggpo;
// GGPOSessionCallbacks cb;
GGPOPlayer p1, p2;
GGPOPlayerHandle player_handles[2];
GGPOPlayerHandle local_player_handle;

GameState stateObj;
FightingGameServer* fgServer;
Character* characters[2];
godot::Input* InputServer;

FightingGameServer::FightingGameServer() { 
  godot::UtilityFunctions::print("engine constructor");
}

FightingGameServer::~FightingGameServer() { 
  godot::UtilityFunctions::print("engine destructor");
}

void FightingGameServer::enter(){
  godot::UtilityFunctions::print(std::filesystem::current_path().c_str());
  shouldUpdate = true;
  fgServer = this;
  characters[0] = &player1;
  characters[1] = &player2;


  player1.otherChar = &player2;
  player2.otherChar = &player1;

  player1.control = 0;
  player2.control = 0;

  std::string p1DefPath = "/Users/martin/dev/godot_projects/template_test/GDExtensionTemplate/data/characters/alucard/def.json";
  std::string p2DefPath = "/Users/martin/dev/godot_projects/template_test/GDExtensionTemplate/data/characters/alucard/def.json";

  player1.init(p1DefPath.c_str());
  player2.init(p2DefPath.c_str());

  player1.loadCustomStates(p2DefPath.c_str());
  player2.loadCustomStates(p1DefPath.c_str());

  player1.virtualController = &p1Vc;
  player1.virtualController->initCommandCompiler(player1.commandPath.c_str());

  player2.virtualController = &p2Vc;
  player2.virtualController->initCommandCompiler(player2.commandPath.c_str());

  camera.init(camWidth, camHeight, worldWidth);
  updateCamera();

  // stateObj.roundStartCounter = 210;
  // stateObj.roundStart = true;
  roundStartCounter = 60;
  roundStart = true;
  slowMode = false;
  if(netPlayState){
    ggpoInit();
  }
}
void FightingGameServer::_ready() {
  if(godot::Engine::get_singleton()->is_editor_hint()){ return; }
  InputServer = godot::Input::get_singleton();
  godot::UtilityFunctions::print("in ready");

  bool _isNetPlay = this->get_parent()->get_meta("isNetPlay");
  netPlayState = _isNetPlay;

  int _netPnum = this->get_parent()->get_meta("netPnum");
  netPnum = _netPnum;

  int _localPort = this->get_parent()->get_meta("localPort");
  localPort = _localPort;

  int _remotePort = this->get_parent()->get_meta("remotePort");
  remotePort = _remotePort;

  std::string _p1Name(this->get_parent()->get_meta("p1Name").stringify().utf8().get_data());
  std::string _p2Name(this->get_parent()->get_meta("p2Name").stringify().utf8().get_data());
  std::string _remoteIp(this->get_parent()->get_meta("remoteIp").stringify().utf8().get_data());
  remoteIp = _remoteIp;

  godot::UtilityFunctions::print(
      "p1:", _p1Name.c_str(), 
      " p2:", _p2Name.c_str(), 
      " netPnum :", _netPnum,
      " isNetPlay:", _isNetPlay,
      " localPort:", _localPort,
      " remotePort:",_remotePort,
      " remoteIp:", _remoteIp.c_str()
      );
  enter();
}

int FightingGameServer::readGodotInputs(int pNum){
  int inputFrame = 0;
  int inputAxisX = 0;
  int inputAxisY = 0;
  std::string prefix = pNum == 1 ? "p1_":"p2_";
  std::string right = prefix + "right";
  std::string left = prefix + "left";
  std::string up = prefix + "up";
  std::string down = prefix + "down";
  std::string buttonA = prefix + "button_A";
  std::string buttonB = prefix + "button_B";
  std::string buttonC = prefix + "button_C";
  std::string buttonD = prefix + "button_D";
  if(InputServer->is_action_pressed(right.c_str())){
    inputAxisX++;
  }
  if(InputServer->is_action_pressed(left.c_str())){
    inputAxisX--;
  }
  if(InputServer->is_action_pressed(up.c_str())){
    inputAxisY++;
  }
  if(InputServer->is_action_pressed(down.c_str())){
    inputAxisY--;
  }
  if(InputServer->is_action_pressed(buttonA.c_str())){
    inputFrame |= LP;
  }
  if(InputServer->is_action_pressed(buttonB.c_str())){
    inputFrame |= LK;
  }
  if(InputServer->is_action_pressed(buttonC.c_str())){
    inputFrame |= MP;
  }
  if(InputServer->is_action_pressed(buttonD.c_str())){
    inputFrame |= MK;
  }

  if(inputAxisX == 1){
    inputFrame |= RIGHT;
  }
  if(inputAxisX == -1){
    inputFrame |= LEFT;
  }
  if(inputAxisY == 1){
    inputFrame |= UP;
  }
  if(inputAxisY == -1){
    inputFrame |= DOWN;
  }
  return inputFrame;
}

void FightingGameServer::readGodotTrainingInput(){
  if(InputServer->is_action_just_released("v_save_state")){
    saveState();
  }
  if(InputServer->is_action_just_released("v_load_state")){
    loadState();
  }
  if(InputServer->is_action_just_released("v_pause_state")){
    pauseState();
  }
  if(InputServer->is_action_just_released("v_unpause_state")){
    unpauseState();
  }
  if(InputServer->is_action_just_released("v_step_state")){
    stepState();
  }
  if(InputServer->is_action_just_released("v_toggle_recording")){
    p1Vc.toggleRecording();
  }
  if(InputServer->is_action_just_released("v_toggle_playback")){
    p1Vc.togglePlayback();
  }
}

void FightingGameServer::_physics_process(double delta) {
  // I hate this
  if(godot::Engine::get_singleton()->is_editor_hint()){ return; }

  int inputs[2] = {0};
  int input = 0;

  if(netPlayState){
    ggpo_idle(ggpo, 1);
    input = readGodotInputs(1);
  } else {
    inputs[0] = readGodotInputs(1);
    inputs[1] = readGodotInputs(2);
    readGodotTrainingInput();
  }
#if defined(SYNC_TEST)
  input = rand(); // test: use random inputs to demonstrate sync testing
#endif

  if(netPlayState){
    int disconnectFlags;
    GGPOErrorCode result = GGPO_OK;
    result = ggpo_add_local_input(ggpo, player_handles[netPnum - 1], &input, sizeof(int));
    if (GGPO_SUCCEEDED(result)) {
      result = ggpo_synchronize_input(ggpo, (void *)inputs, sizeof(int) * 2, &disconnectFlags);
      if (GGPO_SUCCEEDED(result)) {
        step(inputs);
      }
    }

  } else {
    if(shouldUpdate || stepOnce){
      step(inputs);
      if(stepOnce){
        stepOnce = false;
      }
    }
  }
}

void FightingGameServer::_process(double delta) { 
  if(godot::Engine::get_singleton()->is_editor_hint()){ return; }
}

void FightingGameServer::_bind_methods() { 
  godot::ClassDB::bind_method(godot::D_METHOD("getModelName", "p_charNum"), &FightingGameServer::getModelName);
  godot::ClassDB::bind_method(godot::D_METHOD("getModelScale", "p_charNum"), &FightingGameServer::getModelScale);
  godot::ClassDB::bind_method(godot::D_METHOD("getGameState"), &FightingGameServer::getGameState);
}

void FightingGameServer::step(int inputs[]){
  frameCount++;

  p1Vc.update(inputs[0]);
  p2Vc.update(inputs[1]);

  // Handle Input
  if (!slowMode && !screenFreeze) {
    handleRoundStart();

    physics.checkCorner(&player1, worldWidth);
    physics.checkCorner(&player2, worldWidth);

    updateFaceRight();

    checkHitstop(&player1);
    checkHitstop(&player2);

    checkEntityHitstop(&player1);
    checkEntityHitstop(&player2);

    player1.handleInput();
    player2.handleInput();

    for (auto& i : player1.entityList) {
      if (i.active && !i.inHitStop) {
        i.handleInput();
      }
    }

    for (auto& i : player2.entityList) {
      if (i.active && !i.inHitStop) {
        i.handleInput();
      }
    }
  }

  player1.currentState->handleCancels();
  player2.currentState->handleCancels();
  for (auto& i : player1.entityList) {
    if (i.active) {
      i.currentState->handleCancels();
    }
  }
  for (auto& i : player2.entityList) {
    if (i.active) {
      i.currentState->handleCancels();
    }
  }

  checkThrowCollisions();
  physics.checkProximityBox(&player1, &player2);
  physics.checkProximityBox(&player2, &player1);

  physics.checkProjectileBox(&player1, &player2);
  checkHitCollisions();
  physics.checkCorner(&player1, worldWidth);
  physics.checkCorner(&player2, worldWidth);
  physics.checkBounds(&player1, &player2, camera, worldWidth);
  updateFaceRight();

  if (!slowMode && !screenFreeze) {
    if (!player1.inHitStop) {
      player1.update();
    }

    if (!player2.inHitStop) {
      player2.update();
    }

    for (auto& i : player1.entityList) {
      if (i.active && !i.inHitStop) {
        i.update();
      }
    }
    for (auto& i : player2.entityList) {
      if (i.active && !i.inHitStop) {
        i.update();
      }
    }

    if (player1.currentState->checkFlag(SUPER_ATTACK) && (player1.currentState->stateTime == player1.currentState->freezeFrame)) {
      screenFreeze = true;
      screenFreezeLength = player1.currentState->freezeLength;
      player1.activateVisFX(1);
    }

    if (player2.currentState->checkFlag(SUPER_ATTACK) && (player2.currentState->stateTime == player2.currentState->freezeFrame)) {
      screenFreeze = true;
      screenFreezeLength = player2.currentState->freezeLength;
      player2.activateVisFX(1);
    }
  }


  //TODO: Fix redundancies
  physics.checkBounds(&player1, &player2, camera, worldWidth);
  updateFaceRight();
  physics.checkCorner(&player1, worldWidth);
  physics.checkCorner(&player2, worldWidth);

  physics.checkPushBox(&player1, &player2, worldWidth);
  physics.checkBounds(&player1, &player2, camera, worldWidth);

  updateCamera();
  // checkHealth();


  if (slowMode) {
    godot::UtilityFunctions::print("Why the fuck are we in slowmode??:", slowDownCounter);
    printf("in slowMode! slowDownCounter:%d\n", slowDownCounter);
    if (slowDownCounter++ == 70) {
      slowDownCounter = 0;
      slowMode = false;
      roundEnd = true;

      if (roundWinner == 1) {
      }
      else if (roundWinner == 2) {
      }
      roundWinner = 0;
    }
  }

  if (screenFreeze) {
    if (screenFreezeCounter++ == screenFreezeLength) {
      screenFreezeCounter = 0;
      screenFreezeLength = 0;
      screenFreeze = false;
    }
  }
  updateVisuals();
  if(netPlayState){
    ggpo_advance_frame(ggpo);
  }
}


void FightingGameServer::handleRoundStart(){
  if (roundStartCounter > 0) {
    if (--roundStartCounter == 0) {
      player1.control = 1;
      player2.control = 1;
      roundStart = false;
    }
  }
}

void FightingGameServer::checkEntityHitstop(Character* player) {
  for (auto& i : player->entityList) {
    if (i.active && i.inHitStop && --i.hitStop == 0) {
      i.inHitStop = false;
    }
  }
}

void FightingGameServer::updateFaceRight() {
  if (player1.getPos().first == player2.getPos().first) {
  }
  else {
    if (player1.getPos().first < player2.getPos().first) {
      player1.inputFaceRight = true;
      player2.inputFaceRight = false;

      if (!player1.currentState->checkFlag(NO_TURN)) {
        player1.faceRight = true;
      }
      if (!player2.currentState->checkFlag(NO_TURN)) {
        player2.faceRight = false;
      }
    }
    else {
      player1.inputFaceRight = false;
      player2.inputFaceRight = true;

      if (!player1.currentState->checkFlag(NO_TURN)) {
        player1.faceRight = false;
      }
      if (!player2.currentState->checkFlag(NO_TURN)) {
        player2.faceRight = true;
      }
    }
  }

  for (auto& i : player1.entityList) {
    if (i.active && i.updateFacing) {
      if (i.getPos().first < player2.getPos().first) {
        i.inputFaceRight = true;
        if (!i.currentState->checkFlag(NO_TURN)) {
          i.faceRight = true;
        }
      }
      else {
        i.inputFaceRight = false;
        if (!i.currentState->checkFlag(NO_TURN)) {
          i.faceRight = false;
        }
      }
    }
  }

  for (auto& i : player2.entityList) {
    if (i.active && i.updateFacing) {
      if (i.getPos().first < player1.getPos().first) {
        i.inputFaceRight = true;
        if (!i.currentState->checkFlag(NO_TURN)) {
          i.faceRight = true;
        }
      }
      else {
        i.inputFaceRight = false;
        if (!i.currentState->checkFlag(NO_TURN)) {
          i.faceRight = false;
        }
      }
    }
  }
}

void FightingGameServer::updateCamera() {
  camera.update(player1.getPos(), player2.getPos());
}

void FightingGameServer::checkThrowCollisions() {
  ThrowResult p1ThrownState = checkThrowAgainst(&player2, &player1);
  ThrowResult p2ThrownState = checkThrowAgainst(&player1, &player2);

  if (p1ThrownState.thrown && p2ThrownState.thrown) {
    int p1ThrowType = p1ThrownState.throwCb->throwType;
    int p2ThrowType = p2ThrownState.throwCb->throwType;
    if (p1ThrowType == 2 && p2ThrowType == 2) {
      handleSameFrameThrowTech(SS_AIR_THROW_TECH);
    }
    else if (p1ThrowType == 1 && p2ThrowType == 1) {
      handleSameFrameThrowTech(SS_THROW_TECH);
    }
  }
  else if (p1ThrownState.thrown) {
    player1.velocityX = 0;
    player1.velocityY = 0;
    player2.velocityX = 0;
    player2.velocityY = 0;

    player1.changeState(p1ThrownState.throwCb->opponentThrowSuccess);
    player2.changeState(p1ThrownState.throwCb->throwSuccess);
  }
  else if (p2ThrownState.thrown) {
    player1.velocityX = 0;
    player1.velocityY = 0;
    player2.velocityX = 0;
    player2.velocityY = 0;
    player2.changeState(p2ThrownState.throwCb->opponentThrowSuccess);
    player1.changeState(p2ThrownState.throwCb->throwSuccess);
  }
}

void FightingGameServer::checkThrowTechs() {
  if (player1.currentState->checkFlag(TECHABLE)) {
    bool lpPressed = player1._wasPressed(10);
    bool lkPressed = player1._wasPressed(11);
    if (lpPressed && lkPressed) {
      int techState = player1.currentState->techState;
      player1.changeState(techState);
      player2.changeState(techState);

      player1.inHitStop = true;
      player2.inHitStop = true;
      player1.hitStop = 20;
      player2.hitStop = 20;
    }
  }
  if (player2.currentState->checkFlag(TECHABLE)) {
    bool lpPressed = player2._wasPressed(10);
    bool lkPressed = player2._wasPressed(11);
    if (lpPressed && lkPressed) {
      int techState = player2.currentState->techState;
      player1.changeState(techState);
      player2.changeState(techState);

      player1.inHitStop = true;
      player2.inHitStop = true;
      player1.hitStop = 20;
      player2.hitStop = 20;
    }
  }
}

void FightingGameServer::checkHitstop(Character* player) {
  if (player->inHitStop && --player->hitStop == 0) {
    player->inHitStop = false;
  }
}

ThrowResult FightingGameServer::checkThrowAgainst(Character* thrower, Character* throwee) {
  bool canThrow = (thrower->canThrow
      &&thrower->control == true
      && !thrower->currentState->hitboxesDisabled
      && throwee->hitstun <= 0 && throwee->blockstun <= 0
      && !throwee->hurtState(throwee->currentState->stateNum)
      && throwee->throwInvul <= 0);

  if (canThrow) {
    return physics.checkThrowbox(thrower, throwee);
  }
  return ThrowResult{ false, NULL };
}

void FightingGameServer::handleSameFrameThrowTech(SpecialState techState) {
  player1.control = 0;
  player2.control = 0;
  player1.changeState(characters[0]->specialStateMap[techState]);
  player2.changeState(characters[1]->specialStateMap[techState]);
  player1.inHitStop = true;
  player2.inHitStop = true;
  player1.hitStop = 20;
  player2.hitStop = 20;
  // Mix_PlayChannel(0, throwtech, 0);
}

void FightingGameServer::checkHitCollisions() {
  HitResult p2HitState = { false, false, 0, NULL };
  HitResult p1HitState = { false, false, 0, NULL };

  if (!player1.currentState->hitboxesDisabled) {
    p2HitState = physics.checkHitbox(&player1, &player2);
  }

  if (!player2.currentState->hitboxesDisabled) {
    p1HitState = physics.checkHitbox(&player2, &player1);
  }

  if (p1HitState.hit) {
    player1.changeState(p1HitState.hitState);
    if (p1HitState.counter) {
    }
  }
  if (p2HitState.hit) {
    player2.changeState(p2HitState.hitState);
    if (p2HitState.counter) {
    }
  }

  checkEntityHitCollisions();
}

void FightingGameServer::checkEntityHitCollisions() {
  // TODO: func
  HitResult p2Hit = physics.checkEntityHitbox(&player1, &player2);
  HitResult p1Hit = physics.checkEntityHitbox(&player2, &player1);

  if (p1Hit.hit) {
    player1.changeState(p1Hit.hitState);
  }
  if (p2Hit.hit) {
    player2.changeState(p2Hit.hitState);
  }

}
void FightingGameServer::checkHealth() {
  player1.meterArray[2] += (player1.tensionGained - player2.tensionGained);
  player2.meterArray[2] += (player2.tensionGained - player1.tensionGained);
  if (player1.meterArray[2] >= 1000) {
    player1.meterArray[2] = 1000;
    player2.meterArray[2] = 0;
  }

  if (player1.meterArray[2] < 0) {
    player1.meterArray[2] = 0;
    player2.meterArray[2] = 1000;
  }
  if (player1.meterArray[1] > player1.maxComeback) {
    player1.meterArray[1] = player1.maxComeback;
  }
  if (player1.meterArray[1] < 0) {
    player1.meterArray[1] = 0;
  }
  if (player1.meterArray[0] >= player1.maxMeter) {
    player1.meterArray[0] = player1.maxMeter;
  }
  if (player1.meterArray[0] < 0) {
    player1.meterArray[0] = 0;
  }
  if (player2.meterArray[1] > player2.maxComeback) {
    player2.meterArray[1] = player2.maxComeback;
  }
  if (player2.meterArray[2] >= 1000) {
    player2.meterArray[2] = 1000;
    player1.meterArray[2] = 0;
  }

  if (player2.meterArray[2] < 0) {
    player2.meterArray[2] = 0;
    player1.meterArray[2] = 1000;
  }
  if (player2.meterArray[1] < 0) {
    player2.meterArray[1] = 0;
  }
  if (player2.meterArray[0] >= player2.maxMeter) {
    player2.meterArray[0] = player2.maxMeter;
  }

  if (player2.meterArray[0] < 0) {
    player2.meterArray[0] = 0;
  }


  if ((player1.health <= 0 || player2.health <= 0) && (!player1.isDead && !player2.isDead)) {
    // knockoutPopup.setX(camera.middle);
    // knockoutPopup.setY(camera.cameraRect.y);
    // knockoutPopup.setStateTime(0);
    // knockoutPopup.setActive(true);
    // Mix_PlayChannel(0, koSound, 0);
    if (player1.health <= 0 && player1.hitstun >= 1) {
      player1.isDead = true;
      p2RoundsWon++;
      printf("niggas die every day\n");
      roundWinner = 2;
    }
    if (player2.health <= 0 && player2.hitstun >= 1) {
      player2.isDead = true;
      p1RoundsWon++;
      printf("niggas die every other day\n");
      roundWinner = 1;
    }
    currentRound++;
    godot::UtilityFunctions::print("Is this ever happening?!?!");
    slowMode = true;
  }

  if (player1.comboCounter == 0 && player1.health < player1.redHealth) {
    if (player1.redHealthCounter++ == 3) {
      player1.redHealthCounter = 0;
      player1.redHealth -= 10;
    }
  }
  if (player2.comboCounter == 0 && player2.health < player2.redHealth) {
    if (player2.redHealthCounter++ == 3) {
      player2.redHealthCounter = 0;
      player2.redHealth -= 10;
    }
  }

  if (roundEnd) {
    if (slowDownCounter++ == 180) {
      slowDownCounter = 0;
      roundEnd = false;
      if (p2RoundsWon == 2 && p1RoundsWon == 2) {
        p1RoundsWon = 1;
        p2RoundsWon = 1;
      }
      else if (p1RoundsWon == 2) {
        p1RoundsWon = 0;
        p2RoundsWon = 0;
        restartRound();
      }
      else if (p2RoundsWon == 2) {
        p1RoundsWon = 0;
        p2RoundsWon = 0;
        restartRound();
      }
      else {
        restartRound();
      }
    }
  }
}

void FightingGameServer::restartRound(){
  player1.refresh();
  player2.refresh();
  player1.control = 0;
  player2.control = 0;

  player1.setXPos(p1StartPos);
  player1.setYPos(0);

  player2.setXPos(p2StartPos);
  player2.setYPos(0);
  updateCamera();
  roundStartCounter = 210;
  roundStart = true;
}

void FightingGameServer::updateVisuals(){
  player1.hitSpark.update();
  player2.hitSpark.update();
}


//** GODOT FUNCTIONS **//
godot::String FightingGameServer::getModelName(int p_charNum) { 
  return characters[p_charNum]->modelName.c_str();
}

double FightingGameServer::getModelScale(int p_charNum) { 
  return characters[p_charNum]->modelScale;
}

godot::Dictionary FightingGameServer::getGameState() { 
  godot::Dictionary state;

  state["char1FaceRight"] = player1.faceRight;
  state["char1StateNum"] = player1.currentState->stateNum;
  state["char1StateTime"] = player1.currentState->stateTime;
  state["char1PosX"] = player1.position.first;
  state["char1PosY"] = player1.position.second;
  state["char1CurrentAnim"] = player1.currentState->animationPath.c_str();
  state["char1LoopAnimation"] = player1.currentState->loopAnimation;

  godot::Array p1Boxes;
  for(auto collisionBox : player1.currentState->collisionBoxes) {
    godot::Dictionary cbDict;
    cbDict["width"] = collisionBox.width;
    cbDict["height"] = collisionBox.height;
    cbDict["posX"] = collisionBox.positionX;
    cbDict["posY"] = collisionBox.positionY;
    cbDict["type"] = collisionBox.boxType;
    cbDict["disabled"] = collisionBox.disabled;
    p1Boxes.append(cbDict);
  }
  state["p1CollisionBoxes"] = p1Boxes;

  state["char1FireballPosX"] = player1.entityList[0].position.first;
  state["char1FireballPosY"] = player1.entityList[0].position.second;
  godot::Array p1FireballBoxes;
  for(auto collisionBox : player1.entityList[0].currentState->collisionBoxes) {
    godot::Dictionary cbDict;
    cbDict["width"] = collisionBox.width;
    cbDict["height"] = collisionBox.height;
    cbDict["posX"] = collisionBox.positionX;
    cbDict["posY"] = collisionBox.positionY;
    cbDict["type"] = collisionBox.boxType;
    cbDict["disabled"] = collisionBox.disabled;
    p1FireballBoxes.append(cbDict);
  }
  state["p1FireballBoxes"] = p1FireballBoxes;


  state["p1HitsparkActive"] = player1.hitSpark.getActive();
  state["p1HitsparkStatetTime"] = player1.hitSpark.getStateTime();
  state["p1HitsparkX"] = player1.hitSpark.getX();
  state["p1HitsparkY"] = player1.hitSpark.getY();

  // bool isActive = false;
  // int stateTime = 0;
  // int xPos = 0;
  // int yPos = 0;

  state["char2FaceRight"] = player2.faceRight;
  state["char2StateNum"] = player2.currentState->stateNum;
  state["char2StateTime"] = player2.currentState->stateTime;
  state["char2CurrentAnim"] = player2.currentState->animationPath.c_str();
  state["char2LoopAnimation"] = player2.currentState->loopAnimation;
  state["char2PosX"] = player2.position.first;
  state["char2PosY"] = player2.position.second;
  godot::Array p2Boxes;
  for(auto collisionBox : player2.currentState->collisionBoxes) {
    godot::Dictionary cbDict;
    cbDict["width"] = collisionBox.width;
    cbDict["height"] = collisionBox.height;
    cbDict["posX"] = collisionBox.positionX;
    cbDict["posY"] = collisionBox.positionY;
    cbDict["type"] = collisionBox.boxType;
    cbDict["disabled"] = collisionBox.disabled;
    p2Boxes.append(cbDict);
  }
  state["p2CollisionBoxes"] = p2Boxes;

  state["p2HitsparkActive"] = player2.hitSpark.getActive();
  state["p2HitsparkStatetTime"] = player2.hitSpark.getStateTime();
  state["p2HitsparkX"] = player2.hitSpark.getX();
  state["p2HitsparkY"] = player2.hitSpark.getY();

  return state;
}

//** GGPO **//
bool begin_game_callback(const char* message){ 
  godot::UtilityFunctions::print("GGPO SESSION STARTED");
  return true; 
}

bool on_event_callback(GGPOEvent* info){
  int progress;
  switch (info->code) {
    case GGPO_EVENTCODE_CONNECTED_TO_PEER:
      godot::UtilityFunctions::print("GGPO player:",info->u.connected.player, " is connecting");
      // ngs.SetConnectState(info->u.connected.player, Synchronizing);
      break;
    case GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER:
      progress = 100 * info->u.synchronizing.count / info->u.synchronizing.total;
      godot::UtilityFunctions::print("GGPO player:", info->u.connected.player, " progress:",  progress);
      // ngs.UpdateConnectProgress(info->u.synchronizing.player, progress);
      break;
    case GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER:
      godot::UtilityFunctions::print("GGPO player:",info->u.connected.player, " done connecting");
      // ngs.UpdateConnectProgress(info->u.synchronized.player, 100);
      break;
    case GGPO_EVENTCODE_RUNNING:
      godot::UtilityFunctions::print("GGPO running");
      // ngs.SetConnectState(Running);
      // renderer->SetStatusText("");
      break;
    case GGPO_EVENTCODE_CONNECTION_INTERRUPTED:
      godot::UtilityFunctions::print("GGPO connection interrupted");
      // ngs.SetDisconnectTimeout(info->u.connection_interrupted.player, timeGetTime(), info->u.connection_interrupted.disconnect_timeout);
      break;
    case GGPO_EVENTCODE_CONNECTION_RESUMED:
      godot::UtilityFunctions::print("GGPO connection resumed");
      // ngs.SetConnectState(info->u.connection_resumed.player, Running);
      break;
    case GGPO_EVENTCODE_DISCONNECTED_FROM_PEER:
      godot::UtilityFunctions::print("GGPO connection disconnected");
      // ngs.SetConnectState(info->u.disconnected.player, Disconnected);
      break;
    case GGPO_EVENTCODE_TIMESYNC:
      godot::UtilityFunctions::print("GGPO timesync");
#ifdef _WIN32
      Sleep(1000 * info->u.timesync.frames_ahead / 60);
#else
      godot::UtilityFunctions::print("you are ", info->u.timesync.frames_ahead, " frames ahead!");
      // sleep(1000 * info->u.timesync.frames_ahead / 60);
#endif
      break;
  }
  return true;
}

bool advance_frame_callback(int frame){
  int inputs[2] = { 0 };
  int disconnectFlags;
  ggpo_synchronize_input(ggpo, (void*) inputs, sizeof(int) * 2, &disconnectFlags);
  fgServer->step(inputs);
  return true;
}

bool save_state_callback(unsigned char** buffer, int* len, int* checksum, int frame){
  stateObj.roundStartCounter = fgServer->roundStartCounter;
  stateObj.roundStart = fgServer->roundStart;
  stateObj.roundWinner = fgServer->roundWinner;
  stateObj.slowMode = fgServer->slowMode;
  stateObj.frameCount = fgServer->frameCount;
  stateObj.currentRound = fgServer->currentRound;
  stateObj.shouldUpdate = fgServer->shouldUpdate;
  stateObj.slowMode = fgServer->slowMode;
  stateObj.slowDownCounter = fgServer->slowDownCounter;
  stateObj.screenFreeze = fgServer->screenFreeze;
  stateObj.screenFreezeLength = fgServer->screenFreezeLength;
  stateObj.screenFreezeCounter = fgServer->screenFreezeCounter;
  stateObj.netPlayState = fgServer->netPlayState;

  stateObj.player1 = fgServer->player1.saveState();
  stateObj.player2 = fgServer->player2.saveState();
  stateObj.cameraState = fgServer->camera.saveState();

  *len = sizeof(stateObj);
  *buffer = (unsigned char*)malloc(*len);
  if(!*buffer) {
    return false;
  }
  memcpy(*buffer, &stateObj, *len);
  *checksum = Util::fletcher32_checksum((short *)*buffer, *len / 2);
  // godot::UtilityFunctions::print("frameCount:", stateObj.frameCount, " checksum:", *checksum);
  return true;
}

bool load_state_callback(unsigned char* buffer, int len){
  memcpy(&stateObj, buffer, len);
  godot::UtilityFunctions::print("loading frame ", stateObj.frameCount);
  fgServer->roundStartCounter = stateObj.roundStartCounter;
  fgServer->roundStart = stateObj.roundStart;
  fgServer->roundWinner = stateObj.roundWinner;
  fgServer->slowMode = stateObj.slowMode;
  fgServer->frameCount = stateObj.frameCount;
  fgServer->currentRound = stateObj.currentRound;
  fgServer->shouldUpdate = stateObj.shouldUpdate;
  fgServer->slowMode = stateObj.slowMode;
  fgServer->slowDownCounter = stateObj.slowDownCounter;
  fgServer->screenFreeze = stateObj.screenFreeze;
  fgServer->screenFreezeLength = stateObj.screenFreezeLength;
  fgServer->screenFreezeCounter = stateObj.screenFreezeCounter;
  fgServer->netPlayState = stateObj.netPlayState;

  fgServer->player1.loadState(stateObj.player1);
  fgServer->player2.loadState(stateObj.player2);
  fgServer->camera.loadState(stateObj.cameraState);
  return true;
}

bool log_game_state(char* filename, unsigned char* buffer, int frame){
  return true;
}

void free_buffer(void* buffer){
  std::free(buffer);
}


void FightingGameServer::saveState(){
  // why would godot have a function called free in its namespace wtf??
  stateObj.roundStartCounter = roundStartCounter;
  stateObj.roundStart = roundStart;
  stateObj.roundWinner = roundWinner;
  stateObj.slowMode = slowMode;
  stateObj.frameCount = frameCount;
  stateObj.currentRound = currentRound;
  stateObj.shouldUpdate = shouldUpdate;
  stateObj.slowMode = slowMode;
  stateObj.slowDownCounter = slowDownCounter;
  stateObj.screenFreeze = screenFreeze;
  stateObj.screenFreezeLength = screenFreezeLength;
  stateObj.screenFreezeCounter = screenFreezeCounter;
  stateObj.netPlayState = netPlayState;

  stateObj.player1 = player1.saveState();
  stateObj.player2 = player2.saveState();
  stateObj.cameraState = camera.saveState();

  localBufferSize = sizeof(stateObj);
  localBuffer = (unsigned char*)malloc(localBufferSize);
  if(!localBuffer) {
    godot::UtilityFunctions::print("error creating buffer of len:!", localBufferSize);
  } else {
    godot::UtilityFunctions::print("created buffer of len:!", localBufferSize);
    memcpy(localBuffer, &stateObj, localBufferSize);
  }
}

void FightingGameServer::loadState(){
  memcpy(&stateObj, localBuffer, localBufferSize);
  roundStartCounter = stateObj.roundStartCounter;
  roundStart = stateObj.roundStart;
  roundWinner = stateObj.roundWinner;
  slowMode = stateObj.slowMode;
  frameCount = stateObj.frameCount;
  currentRound = stateObj.currentRound;
  shouldUpdate = stateObj.shouldUpdate;
  slowMode = stateObj.slowMode;
  slowDownCounter = stateObj.slowDownCounter;
  screenFreeze = stateObj.screenFreeze;
  screenFreezeLength = stateObj.screenFreezeLength;
  screenFreezeCounter = stateObj.screenFreezeCounter;
  netPlayState = stateObj.netPlayState;

  player1.loadState(stateObj.player1);
  player2.loadState(stateObj.player2);
  camera.loadState(stateObj.cameraState);
}

void FightingGameServer::pauseState(){
  shouldUpdate = false;
}
void FightingGameServer::unpauseState(){
  shouldUpdate = true;
}

void FightingGameServer::stepState(){
  stepOnce = true;
}

int FightingGameServer::getPort(){
  return 9998;
}

std::string FightingGameServer::getIp(){
  return "127.0.0.1";
}

void FightingGameServer::ggpoInit(){
  godot::UtilityFunctions::print("IN GGPO INIT");

  GGPOSessionCallbacks cb;
  cb.on_event = on_event_callback;
  cb.begin_game = begin_game_callback;
  cb.advance_frame = advance_frame_callback;
  cb.load_game_state = load_state_callback;
  cb.save_game_state = save_state_callback;
  cb.free_buffer = free_buffer;
  cb.log_game_state = log_game_state;

  // Start Session
  // GGPOErrorCode result;
#if defined(SYNC_TEST)
  ggpo_start_synctest(&ggpo, &cb, "beatdown", 2, sizeof(int), 1);
#else
  ggpo_start_session(&ggpo, &cb, "beatdown", 2, sizeof(int), localPort);
#endif

  ggpo_set_disconnect_timeout(ggpo, 3000);
  ggpo_set_disconnect_notify_start(ggpo, 1000);


  p1.player_num = 1;
  p1.size = sizeof(p1);

  p2.player_num = 2;
  p2.size = sizeof(p2);

  if (netPnum == 1) {
    p1.type = GGPO_PLAYERTYPE_LOCAL;
    local_player_handle = player_handles[0];

    p2.type = GGPO_PLAYERTYPE_REMOTE;
    strcpy(p2.u.remote.ip_address, remoteIp.c_str());
    p2.u.remote.port = remotePort;
  }
  else {
    p2.type = GGPO_PLAYERTYPE_LOCAL;
    local_player_handle = player_handles[1];

    p1.type = GGPO_PLAYERTYPE_REMOTE;
    strcpy(p1.u.remote.ip_address, remoteIp.c_str());
    p1.u.remote.port = remotePort;
  }

  ggpo_add_player(ggpo, &p1, &player_handles[0]);
  ggpo_add_player(ggpo, &p2, &player_handles[1]);
}
