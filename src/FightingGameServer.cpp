#include <chrono>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/Input.hpp>
#include <bitset>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include "FightingGameServer.h"
#include "ggponet.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#define SYNC_TEST

GGPOSession* ggpo;
// GGPOSessionCallbacks cb;
GGPOPlayer p1, p2;
GGPOPlayerHandle player_handles[2];
GGPOPlayerHandle* local_player_handle;

GameState stateObj;
FightingGameServer* fgServer;
Character* characters[2];
godot::Input* InputServer;

int fletcher32_checksum(short *data, size_t len) {
   int sum1 = 0xffff, sum2 = 0xffff;

   while (len) {
      size_t tlen = len > 360 ? 360 : len;
      len -= tlen;
      do {
         sum1 += *data++;
         sum2 += sum1;
      } while (--tlen);
      sum1 = (sum1 & 0xffff) + (sum1 >> 16);
      sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   }

   /* Second reduction step to reduce sums to 16 bits */
   sum1 = (sum1 & 0xffff) + (sum1 >> 16);
   sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   return sum2 << 16 | sum1;
}

bool blockState(int playerNum, int state) {
  return (state == characters[playerNum]->specialStateMap[SS_BLOCK_STAND]
      || state == characters[playerNum]->specialStateMap[SS_BLOCK_CROUCH]
      || state == characters[playerNum]->specialStateMap[SS_AIR_BLOCK]
      || state == characters[playerNum]->specialStateMap[SS_PUSH_BLOCK]
      || state == characters[playerNum]->specialStateMap[SS_CROUCH_PUSH_BLOCK]
      || state == characters[playerNum]->specialStateMap[SS_AIR_PUSH_BLOCK]
      );
}

bool airHurtState(int playerNum, int state) {
  return (
      state == characters[playerNum]->specialStateMap[SS_AIR_HURT]
      || state == characters[playerNum]->specialStateMap[SS_AIR_HURT_RECOVERY]
      || state == characters[playerNum]->specialStateMap[SS_BLOWBACK_FALLING]
      || state == characters[playerNum]->specialStateMap[SS_DEAD_STANDING]
      || state == characters[playerNum]->specialStateMap[SS_DEAD_KNOCKDOWN]
      || state == characters[playerNum]->specialStateMap[SS_GROUNDBOUNCE_FLING]
      || state == characters[playerNum]->specialStateMap[SS_GROUNDBOUNCE_IMPACT]
      || state == characters[playerNum]->specialStateMap[SS_FLOAT_HURT]
      || state == characters[playerNum]->specialStateMap[SS_FLOAT_HURT_RECOVERY]);
}

FightingGameServer::FightingGameServer() { 
  godot::UtilityFunctions::print("engine constructor");
}

FightingGameServer::~FightingGameServer() { 
  godot::UtilityFunctions::print("engine destructor");
}

void FightingGameServer::enter(){
  godot::UtilityFunctions::print(std::filesystem::current_path().c_str());
  netPlayState = true;
  netPnum = 1;
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
  if(godot::Engine::get_singleton()->is_editor_hint()){
    //man wtf?
    return;
  }
  godot::UtilityFunctions::print("in ready");
  InputServer = godot::Input::get_singleton();
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
  if(InputServer->is_action_just_released("v_toggle_recording")){
    p1Vc.toggleRecording();
  }
  if(InputServer->is_action_just_released("v_toggle_playback")){
    p1Vc.togglePlayback();
  }
}

void FightingGameServer::_physics_process(double delta) {
  if(godot::Engine::get_singleton()->is_editor_hint()){ return; }
  if(netPlayState){
    ggpo_idle(ggpo, 1);
  }
  GGPOErrorCode result = GGPO_OK;
  int disconnectFlags;
  // readGodotTrainingInput();

  int inputs[2] = {0};
  // inputs[0] = readGodotInputs(1);
  // inputs[1] = readGodotInputs(2);
#if defined(SYNC_TEST)
  inputs[0] = 0; // test: use random inputs to demonstrate sync testing
#endif

  if(netPlayState){
    result = ggpo_add_local_input(ggpo, player_handles[0], &inputs[0], sizeof(inputs[0]));
    if (GGPO_SUCCEEDED(result)) {
      result = ggpo_synchronize_input(ggpo, (void *)inputs, sizeof(int) * 2, &disconnectFlags);
      if (GGPO_SUCCEEDED(result)) {
        // inputs[0] and inputs[1] contain the inputs for p1 and p2.  Advance
        // the game by 1 frame using those inputs.
        step(inputs);
      }
    }

  } else {
    step(inputs);
  }

  // auto stop = std::chrono::high_resolution_clock::now();
  // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
  // godot::UtilityFunctions::print("time spent in server:", duration.count());
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
  p1Vc.update(inputs[0]);
  p2Vc.update(inputs[1]);
  frameCount++;


  // Handle Input
  if (!slowMode && !screenFreeze) {
    handleRoundStart();
    checkCorner(&player1);
    checkCorner(&player2);
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
    // checkThrowTechs();
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
  checkProximityAgainst(&player1, &player2);
  checkProximityAgainst(&player2, &player1);
  checkProjectileCollisions(&player1, &player2);
  checkHitCollisions();
  checkTriggerCollisions();
  checkCorner(&player1);
  checkCorner(&player2);
  checkBounds();
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
  checkBounds();
  updateFaceRight();
  checkCorner(&player1);
  checkCorner(&player2);

  checkPushCollisions();
  checkBounds();

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
        // p1WinPopup.setX(camera.middle);
        // p1WinPopup.setY(camera.cameraRect.y);
        // p1WinPopup.setStateTime(0);
        // p1WinPopup.setActive(true);
        // Mix_PlayChannel(0, p1WinSound, 0);
      }
      else if (roundWinner == 2) {
        // p2WinPopup.setX(camera.middle);
        // p2WinPopup.setY(camera.cameraRect.y);
        // p2WinPopup.setStateTime(0);
        // p2WinPopup.setActive(true);
        // Mix_PlayChannel(0, p2WinSound, 0);
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
  // updateVisuals();
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

void FightingGameServer::checkCorner(Character* player) {
  if (player->getPos().first - player->width <= 0 || player->getPos().first + player->width >= worldWidth) {
    player->inCorner = true;
    // godot::UtilityFunctions::print(player->playerNum, " in corner");
  }
  else {
    player->inCorner = false;
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

    // if (player1.control) {
    //   player1.control = 0;
    //   // you were thrown
    //   // TODO: opponentThrowSuccess is a confusing name
    //   // FIXME: CUSTOMSTATE
    //   int customState = p1ThrownState.throwCb->opponentTechAttempt;
    //   player1.changeState(customState);

    //   player2.changeState(p1ThrownState.throwCb->throwAttempt);
    // }
    // else {
    //   // you were thrown
    //   // TODO: opponentThrowSuccess is a confusing name
    //   // FIXME: CUSTOMSTATE
    // }
  }
  else if (p2ThrownState.thrown) {
    player1.velocityX = 0;
    player1.velocityY = 0;
    player2.velocityX = 0;
    player2.velocityY = 0;
    player2.changeState(p2ThrownState.throwCb->opponentThrowSuccess);
    player1.changeState(p2ThrownState.throwCb->throwSuccess);

    // if (player2.control) {
    //   player2.control = 0;

    //   int throwAttempt = p2ThrownState.throwCb->throwAttempt;
    //   int techAttempt = p2ThrownState.throwCb->opponentTechAttempt;

    //   // FIXME: CUSTOMSTATE
    //   player2.changeState(techAttempt);
    //   player1.changeState(throwAttempt);
    // }
    // else {
    //   // FIXME: CUSTOMSTATE
    // }
  }
}

void FightingGameServer::checkPushCollisions() {
  // get the collision box(s) for the current state
  std::pair<int, int> p1Pos = player1.getPos();
  std::pair<int, int> p2Pos = player2.getPos();

  for (auto p1PushBox : player1.currentState->pushBoxes) {
    if (!p1PushBox->disabled) {
      for (auto p2PushBox : player2.currentState->pushBoxes) {
        if (!p2PushBox->disabled) {
          if (CollisionBox::checkAABB(*p1PushBox, *p2PushBox)) {
            // find how deeply intersected they are
            bool p1Lefter = p1Pos.first < p2Pos.first;
            if (p1Pos.first == p2Pos.first) {
              p1Lefter = player1.faceRight;
            }

            if (p1Lefter) {
              int p1RightEdge = p1PushBox->positionX + p1PushBox->width;
              int p2LeftEdge = p2PushBox->positionX;
              int depth = p1RightEdge - p2LeftEdge;

              // account for over bound 
              if ((p2Pos.first + player2.width) + (depth / 2) > worldWidth) {
                int remainder = worldWidth - (p2Pos.first + (depth / 2));
                player2.setXPos(worldWidth - player2.width);
                player1.setX(-depth);
              }
              else if ((p1Pos.first - player1.width) - (depth / 2) < 0) {
                int remainder = p1Pos.first + (depth / 2);
                player1.setXPos(0 + player1.width);
                player2.setX(depth);
              }
              else {
                player2.setX(depth / 2);
                player1.setX(-depth / 2);
              }
            }
            else {
              int p2RightEdge = p2PushBox->positionX + p2PushBox->width;
              int p1LeftEdge = p1PushBox->positionX;
              int depth = p2RightEdge - p1LeftEdge;

              // account for over bound 
              if ((p1Pos.first + player1.width) + (depth / 2) > worldWidth) {
                int remainder = worldWidth - (p1Pos.first + (depth / 2));
                player1.setXPos(worldWidth + player1.width);
                player2.setX(-depth);
              }
              else if ((p2Pos.first - player2.width) - (depth / 2) < 0) {
                int remainder = p2Pos.first + (depth / 2);
                player2.setXPos(0 + player2.width);
                player1.setX(depth);
              }
              else {
                player2.setX(-depth / 2);
                player1.setX(depth / 2);
              }
            }

            player1.updateCollisionBoxPositions();
            player2.updateCollisionBoxPositions();
          }
        }
      }
    }
  }
}

void FightingGameServer::checkBounds() {
  if (player1.getPos().first - player1.width < 0) {
    player1.setXPos(0 + player1.width);
    player1.updateCollisionBoxPositions();
  }
  if (player1.getPos().first - player1.width < camera.lowerBound) {
    player1.setXPos(camera.lowerBound + player1.width);
    player1.updateCollisionBoxPositions();
  }

  if (player1.getPos().first + player1.width > worldWidth) {
    player1.setXPos(worldWidth - player1.width);
    player1.updateCollisionBoxPositions();
  }
  if (player1.getPos().first + player1.width > camera.upperBound) {
    player1.setXPos(camera.upperBound - player1.width);
    player1.updateCollisionBoxPositions();
  }

  for (auto& entity : player1.entityList) {
    if (entity.active && entity.isFireball) {
      int entityX = entity.getPos().first;
      int entityW = entity.width;
      bool lowerBound = (entityX) < 0;
      bool lowerCamBound = (entityX) < camera.lowerBound;
      bool upperBound = (entityX) > worldWidth;
      bool upperCamBound = (entityX) > camera.upperBound;

      if (lowerBound || upperBound) {
        entity.deactivateEntity();
      }
    }
  }

  if (player2.getPos().first - player2.width < 0) {
    player2.setXPos(0 + player2.width);
    player2.updateCollisionBoxPositions();
  }
  if (player2.getPos().first - player2.width < camera.lowerBound) {
    player2.setXPos(camera.lowerBound + player2.width);
    player2.updateCollisionBoxPositions();
  }

  if (player2.getPos().first + player2.width > worldWidth) {
    player2.setXPos(worldWidth - player2.width);
    player2.updateCollisionBoxPositions();
  }
  if (player2.getPos().first + player2.width > camera.upperBound) {
    player2.setXPos(camera.upperBound - player2.width);
    player2.updateCollisionBoxPositions();
  }
  for (auto& entity : player2.entityList) {
    if (entity.active && entity.isFireball) {
      int entityX = entity.getPos().first;
      int entityW = entity.width;
      bool lowerBound = (entityX) < 0;
      bool lowerCamBound = (entityX) < camera.lowerBound;
      bool upperBound = (entityX) > worldWidth;
      bool upperCamBound = (entityX) > camera.upperBound;

      if (lowerBound || lowerCamBound || upperBound || upperCamBound) {
        entity.deactivateEntity();
      }
    }
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

int FightingGameServer::checkProjectileCollisions(Character* player1, Character* player2) {
  for (auto& entity : player1->entityList) {
    if (entity.active && !entity.currentState->hitboxesDisabled) {
      for (auto entityHitbox : entity.currentState->projectileBoxes) {
        bool groupDisabled = entity.currentState->hitboxGroupDisabled[entityHitbox->groupID];
        if (!groupDisabled && !entity.inHitStop) {
          for (auto& otherEntity : player2->entityList) {
            if (otherEntity.active && !otherEntity.currentState->hitboxesDisabled) {
              for (auto otherEntityHitbox : otherEntity.currentState->projectileBoxes) {
                bool groupDisabled = otherEntity.currentState->hitboxGroupDisabled[entityHitbox->groupID];
                if (!groupDisabled && !otherEntity.inHitStop) {
                  if (CollisionBox::checkAABB(*entityHitbox, *otherEntityHitbox)) {
                    CollisionRect hitsparkIntersect = CollisionBox::getAABBIntersect(*entityHitbox, *otherEntityHitbox);
                    entity.inHitStop = true;
                    entity.hitStop = 6;
                    otherEntity.inHitStop = true;
                    otherEntity.hitStop = 6;
                    if (--entity.currentDurability <= 0) {

                      entity.currentState->hitboxGroupDisabled[entityHitbox->groupID] = true;
                      entity.currentState->canHitCancel = true;

                      entity.soundsEffects.at(entityHitbox->hitSoundID).active = true;
                      entity.soundsEffects.at(entityHitbox->hitSoundID).channel = player1->soundChannel + 2;
                    };
                    if (--otherEntity.currentDurability <= 0) {
                      otherEntity.currentState->hitboxGroupDisabled[otherEntityHitbox->groupID] = true;
                      otherEntity.currentState->canHitCancel = true;

                      otherEntity.soundsEffects.at(otherEntityHitbox->hitSoundID).active = true;
                      otherEntity.soundsEffects.at(otherEntityHitbox->hitSoundID).channel = player2->soundChannel + 2;
                    };
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  //StateDef* attackerState = player1->getCurrentState();
  //StateDef* defenderState = player2->getCurrentState();
  // if (!attackerState->hitboxesDisabled) {
  //    CollisionBoxList* projectileBoxes = &attackerState->projectileBoxes;
  //    for (auto& projectileBox : *projectileBoxes) {
  //        bool groupDisabled = attackerState->hitboxGroupDisabled[projectileBox->groupID];
  //            if (!groupDisabled) {
  //                for (auto& defenderEntity : player2->entityList) {
  //                    if (defenderEntity.active && !defenderEntity.currentState->hitboxesDisabled) {
  //                        for (auto& otherEntityHitbox : defenderEntity.currentState->projectileBoxes) {
  //                            bool groupDisabled = otherEntity.currentState->hitboxGroupDisabled[entityHitbox->groupID];
  //                            if (!groupDisabled) {
  //                                if (CollisionBox::checkAABB(*entityHitbox, *otherEntityHitbox)) {
  //                                    CollisionRect hitsparkIntersect = CollisionBox::getAABBIntersect(*entityHitbox, *otherEntityHitbox);
  //                                    entity.inHitStop = true;
  //                                    entity.hitStop = 6;
  //                                    otherEntity.inHitStop = true;
  //                                    otherEntity.hitStop = 6;
  //                                    entity.currentState->hitboxGroupDisabled[entityHitbox->groupID] = true;
  //                                    entity.currentState->canHitCancel = true;
  //                                    otherEntity.currentState->hitboxGroupDisabled[entityHitbox->groupID] = true;
  //                                    otherEntity.currentState->canHitCancel = true;
  //                                }
  //                            }
  //                        }
  //                    }
  //               }
  //            }
  //        }
  // }
  return 0;
}

int FightingGameServer::checkProximityAgainst(Character* hitter, Character* hurter) {
  if (hurter->getPos().second <= 0) {
    if (!hitter->currentState->hitboxesDisabled) {
      for (auto hitBox : hitter->currentState->proximityBoxes) {
        bool groupDisabled = hitter->currentState->hitboxGroupDisabled[hitBox->groupID];
        if (!hitBox->disabled && !groupDisabled) {
          for (auto hurtBox : hurter->currentState->hurtBoxes) {
            if (!hurtBox->disabled && !groupDisabled) {
              if (CollisionBox::checkAABB(*hitBox, *hurtBox)) {
                if (hurter->currentState->stateNum == hurter->specialStateMap[SS_WALK_B]) {
                  hurter->changeState(hurter->specialStateMap[SS_BLOCK_STAND]);
                }
                if (hurter->currentState->stateNum == hurter->specialStateMap[SS_CROUCH] && hurter->_getInput(1)) {
                  hurter->changeState(hurter->specialStateMap[SS_BLOCK_CROUCH]);
                }
              }
            }
          }
        }
      }
    }
    for (auto& entity : hitter->entityList) {
      if (entity.active && !entity.currentState->hitboxesDisabled) {
        for (auto entityHitbox : entity.currentState->proximityBoxes) {
          bool groupDisabled = entity.currentState->hitboxGroupDisabled[entityHitbox->groupID];
          if (!entityHitbox->disabled && !groupDisabled) {
            for (auto p2HurtBox : hurter->currentState->hurtBoxes) {
              if (!p2HurtBox->disabled && !entity.inHitStop) {
                if (CollisionBox::checkAABB(*entityHitbox, *p2HurtBox)) {
                  if (hurter->currentState->stateNum == hurter->specialStateMap[SS_WALK_B]) {
                    hurter->changeState(hurter->specialStateMap[SS_BLOCK_STAND]);
                  }
                  if (hurter->currentState->stateNum == hurter->specialStateMap[SS_CROUCH] && hurter->_getInput(1)) {
                    hurter->changeState(hurter->specialStateMap[SS_BLOCK_CROUCH]);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return 0;
}

ThrowResult FightingGameServer::checkThrowAgainst(Character* thrower, Character* throwee) {
  ThrowResult throwResult = ThrowResult{ false, NULL };
  bool canThrow = (thrower->canThrow
      &&thrower->control == true
      && !thrower->currentState->hitboxesDisabled
      && throwee->hitstun <= 0 && throwee->blockstun <= 0
      && !throwee->hurtState(throwee->currentState->stateNum)
      && throwee->throwInvul <= 0);

  if (canThrow) {
    for (auto p1ThrowHitbox : thrower->currentState->throwHitBoxes) {
      if (!p1ThrowHitbox->disabled) {
        for (auto p2HurtBox : throwee->currentState->throwHurtBoxes) {
          if (!p2HurtBox->disabled) {
            if (CollisionBox::checkAABB(*p1ThrowHitbox, *p2HurtBox)) {
              if (p1ThrowHitbox->throwType == 1 && throwee->_getYPos() > 0) {
                throwResult.thrown = true;
                throwResult.throwCb = p1ThrowHitbox;
                thrower->frameLastAttackConnected = frameCount;
                thrower->currentState->hitboxesDisabled = true;
              }
              else if (p1ThrowHitbox->throwType == 2 && throwee->_getYPos() == 0) {
                godot::UtilityFunctions::print("frame: ", thrower->currentState->stateTime," throws, conrol:", thrower->control);
                throwResult.thrown = true;
                throwResult.throwCb = p1ThrowHitbox;
                thrower->frameLastAttackConnected = frameCount;
                thrower->currentState->hitboxesDisabled = true;
              }
            }
          }
        }
      }
    }
  }
  return throwResult;
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
  HitResult p2HitState = checkHitboxAgainstHurtbox(&player1, &player2);
  HitResult p1HitState = checkHitboxAgainstHurtbox(&player2, &player1);

  if (p1HitState.hit) {
    player1.changeState(p1HitState.hitState);
    if (p1HitState.counter) {
      // p1CounterHit.setStateTime(0);
      // p1CounterHit.setActive(true);
    }
  }
  if (p2HitState.hit) {
    player2.changeState(p2HitState.hitState);
    if (p2HitState.counter) {
      // p2CounterHit.setStateTime(0);
      // p2CounterHit.setActive(true);
    }
  }

  checkEntityHitCollisions();
}

TriggerResult FightingGameServer::checkTriggerAgainst(Character* owner, Character* activator) {
  if (!owner->currentState->hitboxesDisabled) {
    for (auto triggerBox : owner->currentState->triggerBoxes) {
      bool groupDisabled = owner->currentState->hitboxGroupDisabled[triggerBox->groupID];
      if (!triggerBox->disabled && !groupDisabled) {
        for (auto hitbox : activator->currentState->hitBoxes) {
          bool groupDisabled = activator->currentState->hitboxGroupDisabled[hitbox->groupID];
          if (!hitbox->disabled && !groupDisabled) {
            if (CollisionBox::checkAABB(*triggerBox, *hitbox)) {
              //TODO: SHAKING SCRIPT
              // shakeCamera(16, &camera);
              CollisionRect hitsparkIntersect = CollisionBox::getAABBIntersect(*triggerBox, *hitbox);
              owner->inHitStop = true;
              owner->hitStop = 16;

              activator->inHitStop = true;
              activator->hitStop = 16;
              activator->frameLastAttackConnected = frameCount;
              // TODO: Hitbox group IDs
              activator->currentState->hitboxGroupDisabled[hitbox->groupID] = true;
              owner->currentState->canHitCancel = true;

              int xEdge = owner->faceRight ? hitsparkIntersect.x + hitsparkIntersect.w : hitsparkIntersect.x;
              int visualID = hitbox->guardsparkID;
              // VisualEffect& visFX = owner->guardSparks.at(visualID);
              // visFX.reset(xEdge, (hitsparkIntersect.y - (hitsparkIntersect.h / 2)));
              // visFX.setActive(true);
              // owner->soundsEffects.at(hitbox->guardSoundID).active = true;
              // owner->soundsEffects.at(hitbox->guardSoundID).channel = owner->soundChannel + 2;

              return { true, triggerBox };
            }
          }
        }
      }
    }
  }

  return { false, NULL };
}

HitResult FightingGameServer::checkHitboxAgainstHurtbox(Character* hitter, Character* hurter) {
  if (!hitter->currentState->hitboxesDisabled) {
    for (auto hitBox : hitter->currentState->hitBoxes) {
      bool groupDisabled = hitter->currentState->hitboxGroupDisabled[hitBox->groupID];
      int blocktype = hitBox->blockType;
      if (!hitBox->disabled && !groupDisabled) {
        for (auto hurtBox : hurter->currentState->hurtBoxes) {
          if (!hurtBox->disabled && !groupDisabled) {
            if (CollisionBox::checkAABB(*hitBox, *hurtBox)) {
              //TODO: SHAKING SCRIPT
              // shakeCamera(hitBox->hitstop, &camera);
              CollisionRect hitsparkIntersect = CollisionBox::getAABBIntersect(*hitBox, *hurtBox);
              hitter->inHitStop = true;
              hitter->hitStop = hitBox->hitstop;

              hurter->hitStop = hitBox->hitstop;
              hurter->inHitStop = true;
              hitter->frameLastAttackConnected = frameCount;
              // TODO: Hitbox group IDs
              hitter->currentState->hitboxGroupDisabled[hitBox->groupID] = true;
              hitter->currentState->canHitCancel = true;
              hitter->_addMeter(hitBox->hitMeterGain);
              if (hurter->blockstun <= 0 && hurter->hitstun <= 0) {
                hitter->tensionGained += 100;
              }
              hurter->_addMeter(hitBox->hitstun);
              int hurterCurrentState = hurter->currentState->stateNum;
              bool blocking = blockState(hurter->playerNum - 1, hurterCurrentState);
              if ((blocking && blocktype == 1)
                  || (blocking && checkBlock(blocktype, hurter))
                  || (hurter->control && checkBlock(blocktype, hurter))) {
                bool instantBlocked = hurter->_checkCommand(11);
                int realBlockstun = hitBox->blockstun;
                if ((hurter->_getYPos() > 0) && (hitter->_getYPos() > 0)) {
                  realBlockstun = hitBox->blockstun - 4;
                }
                if (instantBlocked) {
                  hurter->isLight = true;
                  hurter->tensionGained += 50;
                  // Mix_PlayChannel(0, instantBlock, 0);
                  realBlockstun -= 4;
                  realBlockstun = realBlockstun <= 0 ? 1 : realBlockstun;
                  hurter->blockstun = realBlockstun;
                }
                else {
                  hurter->blockstun = realBlockstun;
                }
                if (hurterCurrentState == hurter->specialStateMap[SS_PUSH_BLOCK]
                    || hurterCurrentState == hurter->specialStateMap[SS_CROUCH_PUSH_BLOCK]
                    || hurterCurrentState == hurter->specialStateMap[SS_AIR_PUSH_BLOCK]) {
                  hurter->isGreen = true;
                  hurter->blockstun = realBlockstun + 4;
                }
                hurter->control = 0;
                bool holdingButtons = hurter->_getInput(11) && hurter->_getInput(12);
                bool downBack = hurter->_getInput(1);
                bool backOrUpback = (hurter->_getInput(4) || hurter->_getInput(7)) && !downBack;
                bool anyBack = (hurter->_getInput(1) || hurter->_getInput(4) || hurter->_getInput(7));

                bool crouchPB = holdingButtons && downBack;
                bool standPB = holdingButtons && backOrUpback;
                if (hurter->_getYPos() > 0) {
                  if (anyBack && holdingButtons) {
                    // Mix_PlayChannel(0, pushBlock, 0);
                    hurter->changeState(hurter->specialStateMap[SS_AIR_PUSH_BLOCK]);
                    hurter->_subtractMeter(1020);
                  }
                  else {
                    hurter->changeState(hurter->specialStateMap[SS_AIR_BLOCK]);
                  }
                }
                else {
                  switch (hitBox->blockType) {
                    case 1:
                      if (hurter->_getInput(1)) {
                        if (crouchPB) {
                          // Mix_PlayChannel(0, pushBlock, 0);
                          hurter->changeState(hurter->specialStateMap[SS_CROUCH_PUSH_BLOCK]);
                          hurter->_subtractMeter(1020);
                        }
                        else {
                          hurter->changeState(hurter->specialStateMap[SS_BLOCK_CROUCH]);
                        }
                      }
                      else {
                        if (standPB) {
                          hurter->_subtractMeter(1020);
                          // Mix_PlayChannel(0, pushBlock, 0);
                          hurter->changeState(hurter->specialStateMap[SS_PUSH_BLOCK]);
                        }
                        else {
                          hurter->changeState(hurter->specialStateMap[SS_BLOCK_STAND]);
                        }
                      }
                      break;
                    case 2:
                      if (crouchPB) {
                        // Mix_PlayChannel(0, pushBlock, 0);
                        hurter->_subtractMeter(1020);
                        hurter->changeState(hurter->specialStateMap[SS_CROUCH_PUSH_BLOCK]);
                      }
                      else {
                        hurter->changeState(hurter->specialStateMap[SS_BLOCK_CROUCH]);
                      }
                      break;
                    case 3:
                      if (standPB) {
                        // Mix_PlayChannel(0, pushBlock, 0);
                        hurter->_subtractMeter(1020);
                        hurter->changeState(hurter->specialStateMap[SS_PUSH_BLOCK]);
                      }
                      hurter->changeState(hurter->specialStateMap[SS_BLOCK_STAND]);
                      break;
                      // should throw error here?
                    default: break;
                  }
                }
                int realPushback = hitBox->pushback;
                if (hurter->currentState->stateNum == hurter->specialStateMap[SS_PUSH_BLOCK]
                    || hurter->currentState->stateNum == hurter->specialStateMap[SS_CROUCH_PUSH_BLOCK]
                    || hurter->currentState->stateNum == hurter->specialStateMap[SS_AIR_PUSH_BLOCK]) {
                  realPushback *= 2;
                } else {
                  // hurter->meterArray[3] += hitBox->riskRaise;
                  // if (hurter->meterArray[3] > hurter->riskMax) {
                  //   hurter->meterArray[3] = hurter->riskMax;
                  // }
                  // printf("hurter risk:%d", hurter->meterArray[3]);
                }

                if (hurter->inCorner && (hitBox->pushback > 0)) {
                  hitter->pushTime = hitBox->blockstun;
                  if (hitter->faceRight) {
                    hitter->pushBackVelocity = realPushback;
                  }
                  else {
                    hitter->pushBackVelocity = -(realPushback);
                  }
                } else {
                  hurter->pushTime = hitBox->pushTime;
                  if (hitter->faceRight) {
                    hurter->pushBackVelocity = -realPushback;
                  }
                  else {
                    hurter->pushBackVelocity = realPushback;
                  }
                }

                int xEdge = hurter->faceRight ? hitsparkIntersect.x + hitsparkIntersect.w : hitsparkIntersect.x;
                int visualID = hitBox->guardsparkID;
                // VisualEffect& visFX = hurter->guardSparks.at(visualID);
                //d visFX.reset(xEdge, (hitsparkIntersect.y - (hitsparkIntersect.h / 2)));
                // visFX.setActive(true);
                // hurter->soundsEffects.at(hitBox->guardSoundID).active = true;
                // hurter->soundsEffects.at(hitBox->guardSoundID).channel = hurter->soundChannel + 2;
              }
              else {
                int finalHitPush = 1;
                if (hurter->timeInHitstun > 600) {
                  finalHitPush = 3;
                  hurter->hurtGravity = hurter->gravityVal * 3;
                }
                else if (hurter->timeInHitstun > 300) {
                  finalHitPush = 2;
                  hurter->hurtGravity = hurter->gravityVal * 2;
                }
                if (hurter->inCorner) {
                  hitter->pushTime = hitBox->hitPushTime;
                  if (hitter->faceRight) {
                    hitter->pushBackVelocity = hitBox->hitVelocityX * finalHitPush;
                  }
                  else {
                    hitter->pushBackVelocity = -(hitBox->hitVelocityX * finalHitPush);
                  }
                }
                else {
                  hurter->hitPushTime = hitBox->hitPushTime;
                  if (hitter->faceRight) {
                    hurter->hitPushVelX = -(hitBox->hitVelocityX * finalHitPush);
                  }
                  else {
                    hurter->hitPushVelX = hitBox->hitVelocityX * finalHitPush;
                  }
                }
                hurter->comboCounter++;
                bool wasACounter = hurter->currentState->counterHitFlag || hurter->meterArray[3] >= 50;
                hurter->currentState->counterHitFlag = false;
                if (wasACounter) {
                  hurter->isRed = true;
                  // Mix_PlayChannel(0, countah, 0);
                }
                if (hurter->comboCounter == 1) {
                  hurter->comboProration = hitBox->initialProration;
                }
                int xEdge = hitter->faceRight ? hitsparkIntersect.x + hitsparkIntersect.w : hitsparkIntersect.x;
                int visualID = hitBox->hitsparkID;
                // VisualEffect& visFX = hitter->hitSparks.at(visualID);
                // visFX.reset(xEdge, (hitsparkIntersect.y - (hitsparkIntersect.h / 2)));
                // visFX.setActive(true);

                hurter->control = 0;
                int finalHitstun = wasACounter ? (hitBox->hitstun + 4) : (hitBox->hitstun);
                finalHitstun += (hurter->_getYPos() > 0) ? 4 : 0;
                if (hurter->timeInHitstun > 600) {
                  finalHitstun *= .3;
                }
                else if (hurter->timeInHitstun > 300) {
                  finalHitstun *= .6;
                }
                else if (hurter->timeInHitstun > 180) {
                  finalHitstun *= .9;
                }
                if (finalHitstun < 1) {
                  finalHitstun = 1;
                }
                int finalDamage = wasACounter ? (hitBox->damage + (hitBox->damage * .5)) : (hitBox->damage);
                // apply character defense modifier
                // (((100*95)/100) * (95/100) * (80/100) * (95/100))
                finalDamage = ((finalDamage * hurter->defenseValue) / 100);
                hurter->meterArray[3] -= hitBox->riskLower;
                if (hurter->meterArray[3] <= 25) {
                  hurter->meterArray[3] = 0;
                  finalDamage = ((finalDamage * hurter->riskScaling) / 100);

                }
                if (hurter->comboCounter > 1) {
                  finalDamage = ((finalDamage * hurter->comboProration) / 100);

                }
                if (hurter->comboScale < 100) {
                  finalDamage = ((finalDamage * hurter->comboScale) / 100);
                }
                hurter->comboScale -= hitBox->scaleLower;
                hurter->comboDamage += finalDamage;
                hurter->health -= finalDamage;
                hurter->hitstun = finalHitstun;

                // hurter->hurtSoundEffects.at(hurter->currentHurtSoundID).active = true;
                // hitter->soundsEffects.at(hitBox->hitSoundID).active = true;
                // hitter->soundsEffects.at(hitBox->hitSoundID).channel = hitter->soundChannel + 2;

                int hurterCurrentState = hurter->currentState->stateNum;
                if ((hitBox->hitType == LAUNCHER)
                    || hitBox->hitType == FLOATER
                    || hitBox->hitType == GROUND_BOUNCE
                    || hurter->_getYPos() > 0
                    || airHurtState(hurter->playerNum - 1, hurterCurrentState)) {
                  if (hitBox->airHitstun > 0) {
                    hurter->hitstun = hitBox->airHitstun;
                  }
                  if (hitBox->airHitVelocityX > 0) {
                    if (hurter->inCorner && (hitBox->pushback > 0)) {
                      hitter->pushTime = hitBox->hitPushTime;
                      if (hitter->faceRight) {
                        hitter->pushBackVelocity = hitBox->airHitVelocityX / 2;
                      }
                      else {
                        hitter->pushBackVelocity = -(hitBox->airHitVelocityX / 2);
                      }
                    }
                    else {
                      hurter->hitPushTime = hitBox->airHitPushTime > 0 ? hitBox->airHitPushTime : hitBox->hitPushTime;
                      if (hitter->faceRight) {
                        hurter->hitPushVelX = -hitBox->airHitVelocityX;
                      }
                      else {
                        hurter->hitPushVelX = hitBox->airHitVelocityX;
                      }
                    }
                  }

                  hurter->velocityY = hitBox->hitVelocityY;
                  SpecialState hurtState;
                  switch (hitBox->hitType) {
                    case LAUNCHER:
                      hurtState = SS_AIR_HURT;
                      break;
                    case FLOATER:
                      hurtState = SS_FLOAT_HURT;
                      break;
                    case GROUND_BOUNCE:
                      hurtState = SS_GROUNDBOUNCE_FLING;
                      break;
                    default:
                      hurtState = SS_AIR_HURT;
                      break;
                  }
                  return { true, wasACounter, hurter->specialStateMap[hurtState], NULL };
                }
                else {
                  return { true, wasACounter, hurter->specialStateMap[SS_HURT], NULL };
                }
              }
            }
          }
        }
      }
    }
  }
  return { false, false, 0, NULL };
}

bool FightingGameServer::checkBlock(int blockType, Character* player) {
  bool isHoldingDownBack = player->_getInput(1);
  bool isHoldingBack = player->_getInput(4);
  bool upBackinScrub = player->_getInput(7);
  // I know, enum
  if (player->_getYPos() > 0) {
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

void FightingGameServer::checkEntityHitCollisions() {
  // TODO: func
  HitResult p2Hit = checkEntityHitAgainst(&player1, &player2);
  HitResult p1Hit = checkEntityHitAgainst(&player2, &player1);

  if (p1Hit.hit) {
    player1.changeState(p1Hit.hitState);
  }
  if (p2Hit.hit) {
    player2.changeState(p2Hit.hitState);
  }

}

void FightingGameServer::checkTriggerCollisions() {
  TriggerResult p2State = checkTriggerAgainst(&player1, &player2);
  TriggerResult p1State = checkTriggerAgainst(&player2, &player1);
  if (p2State.triggered) {
    player1.cancelState(p2State.triggerCb->selfState);
    player2.changeState(p2State.triggerCb->activatorState);
  }
  if (p1State.triggered) {

    player2.cancelState(p1State.triggerCb->selfState);
    player1.changeState(p1State.triggerCb->activatorState);
  }
}

HitResult FightingGameServer::checkEntityHitAgainst(Character* p1, Character* p2) {
  bool p2Hit = false;
  for (auto& entity : p1->entityList) {
    if (entity.active && !entity.currentState->hitboxesDisabled) {
      for (auto entityHitbox : entity.currentState->projectileBoxes) {
        bool groupDisabled = entity.currentState->hitboxGroupDisabled[entityHitbox->groupID];
        if (!entityHitbox->disabled && !groupDisabled) {
          for (auto p2HurtBox : p2->currentState->hurtBoxes) {
            if (!p2HurtBox->disabled && !entity.inHitStop) {
              if (CollisionBox::checkAABB(*entityHitbox, *p2HurtBox)) {
                // shakeCamera(entityHitbox->hitstop, &camera);
                printf("found entity hit\n");
                godot::UtilityFunctions::print("found entity hit");
                CollisionRect hitsparkIntersect = CollisionBox::getAABBIntersect(*entityHitbox, *p2HurtBox);
                bool entityFaceRight = entity.faceRight;
                entity.inHitStop = true;
                if (entityHitbox->selfHitstop > 0) {
                  entity.hitStop = entityHitbox->selfHitstop;
                }
                else {
                  entity.hitStop = entityHitbox->hitstop;
                }

                p2->inHitStop = true;
                p2->hitStop = entityHitbox->hitstop;

                p1->frameLastAttackConnected = frameCount;
                printf("entity durability:%d \n", entity.currentDurability);
                godot::UtilityFunctions::print("entity durability:", entity.currentDurability);
                if (--entity.currentDurability <= 0) {
                  entity.currentState->hitboxGroupDisabled[entityHitbox->groupID] = true;
                  entity.currentState->canHitCancel = true;
                };

                int p2StateNum = p2->currentState->stateNum;

                printf("got to the entity here\n");
                if ((blockState(1, p2StateNum)) || (p2->control && checkBlock(entityHitbox->blockType, p2))) {
                  p2->control = 0;
                  bool instantBlocked = p2->_checkCommand(11);
                  if (instantBlocked) {
                    p2->isLight = true;
                    // Mix_PlayChannel(0, instantBlock, 0);
                    p2->tensionGained += 100;
                    int realBlockstun = entityHitbox->blockstun - 4;
                    realBlockstun = realBlockstun <= 0 ? 1 : realBlockstun;
                    p2->blockstun = realBlockstun;
                  }
                  else {
                    p2->blockstun = entityHitbox->blockstun;
                  }
                  if (p2->_getYPos() > 0) {
                    // TODO: air blocking state
                    p2->changeState(p2->specialStateMap[SS_AIR_BLOCK]);
                  }
                  else {
                    switch (entityHitbox->blockType) {
                      case 1:
                        if (p2->_getInput(1)) {
                          p2->changeState(p2->specialStateMap[SS_BLOCK_CROUCH]);
                        }
                        else {
                          p2->changeState(p2->specialStateMap[SS_BLOCK_STAND]);
                        }
                        break;
                      case 2:
                        p2->changeState(p2->specialStateMap[SS_BLOCK_CROUCH]);
                        break;
                      case 3:
                        p2->changeState(p2->specialStateMap[SS_BLOCK_STAND]);
                        break;
                        // should throw error here
                      default: break;
                    }
                  }

                  p2->pushTime = entityHitbox->pushTime;
                  if (p2->faceRight == entityFaceRight) {
                    if (p2->faceRight) {
                      p2->pushBackVelocity = -entityHitbox->pushback;
                    }
                    else {
                      p2->pushBackVelocity = entityHitbox->pushback;
                    }
                  }
                  else {
                    if (p2->faceRight) {
                      p2->pushBackVelocity = entityHitbox->pushback;
                    }
                    else {
                      p2->pushBackVelocity = -entityHitbox->pushback;
                    }
                  }
                  printf("got to the entity visfx\n");
                  int xEdge = p2->faceRight ? hitsparkIntersect.x + hitsparkIntersect.w : hitsparkIntersect.x;
                  int visualID = entityHitbox->guardsparkID;
                  // VisualEffect& visFX = p2->guardSparks.at(visualID);
                  // visFX.reset(xEdge, (hitsparkIntersect.y - (hitsparkIntersect.h / 2)));
                  // visFX.setActive(true);

                  // p2->soundsEffects.at(entityHitbox->guardSoundID).active = true;
                  // p2->soundsEffects.at(entityHitbox->guardSoundID).channel = p2->soundChannel + 2;
                }
                else {

                  printf("bug to the entity here\n");
                  p2->hitPushTime = entityHitbox->hitPushTime;
                  if (p2->faceRight == entityFaceRight) {
                    if (p2->faceRight) {
                      p2->hitPushVelX = -entityHitbox->hitVelocityX;
                    }
                    else {
                      p2->hitPushVelX = entityHitbox->hitVelocityX;
                    }
                  }
                  else {
                    if (p2->faceRight) {
                      p2->hitPushVelX = entityHitbox->hitVelocityX;
                    }
                    else {
                      p2->hitPushVelX = -entityHitbox->hitVelocityX;
                    }
                  }
                  bool wasACounter = p2->currentState->counterHitFlag;
                  p2->currentState->counterHitFlag = false;
                  if (wasACounter) {
                    // Mix_PlayChannel(0, countah, 0);
                  }

                  int xEdge = entity.faceRight ? hitsparkIntersect.x + hitsparkIntersect.w : hitsparkIntersect.x;
                  int visualID = entityHitbox->hitsparkID;

                  printf("got to the entity hit visfx\n");
                  // VisualEffect& visFX = entity.hitSparks.at(visualID);
                  // visFX.reset(xEdge, (hitsparkIntersect.y - (hitsparkIntersect.h / 2)));
                  // visFX.setActive(true);

                  printf("got to the entity hurt sound\n");

                  p2->control = 0;
                  p2->health -= entityHitbox->damage;
                  p2->hitstun = entityHitbox->hitstun;
                  p2->comboCounter++;
                  // p2->hurtSoundEffects.at(p2->currentHurtSoundID).active = true;
                  // entity.soundsEffects.at(entityHitbox->hitSoundID).active = true;
                  // entity.soundsEffects.at(entityHitbox->hitSoundID).channel = p1->soundChannel + 2;

                  if (entityHitbox->hitType == LAUNCHER
                      || p2->_getYPos() > 0
                      || p2->currentState->stateNum == p2->specialStateMap[SS_AIR_HURT]) {
                    p2->velocityY = entityHitbox->hitVelocityY;
                    printf("got to the launcher return");
                    return { true, false, p2->specialStateMap[SS_AIR_HURT], NULL };
                  }
                  else {

                    printf("got to the normal return\n");
                    return { true, false, p2->specialStateMap[SS_HURT], NULL };
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  return { false, false, 0, NULL };
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
    cbDict["width"] = collisionBox->width;
    cbDict["height"] = collisionBox->height;
    cbDict["posX"] = collisionBox->positionX;
    cbDict["posY"] = collisionBox->positionY;
    cbDict["type"] = collisionBox->boxType;
    cbDict["disabled"] = collisionBox->disabled;
    p1Boxes.append(cbDict);
  }
  state["p1CollisionBoxes"] = p1Boxes;

  state["char1FireballPosX"] = player1.entityList[0].position.first;
  state["char1FireballPosY"] = player1.entityList[0].position.second;
  godot::Array p1FireballBoxes;
  for(auto collisionBox : player1.entityList[0].currentState->collisionBoxes) {
    godot::Dictionary cbDict;
    cbDict["width"] = collisionBox->width;
    cbDict["height"] = collisionBox->height;
    cbDict["posX"] = collisionBox->positionX;
    cbDict["posY"] = collisionBox->positionY;
    cbDict["type"] = collisionBox->boxType;
    cbDict["disabled"] = collisionBox->disabled;
    p1FireballBoxes.append(cbDict);
  }
  state["p1FireballBoxes"] = p1FireballBoxes;

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
    cbDict["width"] = collisionBox->width;
    cbDict["height"] = collisionBox->height;
    cbDict["posX"] = collisionBox->positionX;
    cbDict["posY"] = collisionBox->positionY;
    cbDict["type"] = collisionBox->boxType;
    cbDict["disabled"] = collisionBox->disabled;
    p2Boxes.append(cbDict);
  }
  state["p2CollisionBoxes"] = p2Boxes;
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
      sleep(1000 * info->u.timesync.frames_ahead / 60);
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
  *checksum = fletcher32_checksum((short *)*buffer, *len / 2);
  godot::UtilityFunctions::print("frameCount:", stateObj.frameCount, " checksum:", *checksum);
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

int FightingGameServer::getPort(){
  return 9998;
}

std::string FightingGameServer::getIp(){
  return "127.0.0.1";
}

void FightingGameServer::ggpoInit(){
  godot::UtilityFunctions::print("IN GGPO INIT");
  p1.player_num = 1;
  p1.size = sizeof(p1);

  p2.player_num = 2;
  p2.size = sizeof(p2);

  localPort = getPort();
  localIp = getIp();

#if defined(SYNC_TEST)
  remoteIp = "1.1.1.1";
  remotePort = 9999;
#else
  // get remote IP & remote port from godot somehow
#endif

  godot::UtilityFunctions::print("remote address (", remoteIp.c_str(), ":", remotePort, ")");
  godot::UtilityFunctions::print("local address (", localIp.c_str(), ":", localPort, ")");

  if (netPnum == 1) {
    p1.type = GGPO_PLAYERTYPE_LOCAL;
    local_player_handle = &player_handles[0];

    p2.type = GGPO_PLAYERTYPE_REMOTE;
    strcpy(p2.u.remote.ip_address, remoteIp.c_str());
    p2.u.remote.port = remotePort;
  }
  else {
    p2.type = GGPO_PLAYERTYPE_LOCAL;
    local_player_handle = &player_handles[1];

    p1.type = GGPO_PLAYERTYPE_REMOTE;
    strcpy(p1.u.remote.ip_address, remoteIp.c_str());
    p1.u.remote.port = remotePort;
  }

  GGPOSessionCallbacks cb;
  cb.on_event = on_event_callback;
  cb.begin_game = begin_game_callback;
  cb.advance_frame = advance_frame_callback;
  cb.load_game_state = load_state_callback;
  cb.save_game_state = save_state_callback;
  cb.free_buffer = free_buffer;
  cb.log_game_state = log_game_state;

  // Start Session
  GGPOErrorCode result;
#if defined(SYNC_TEST)
  result = ggpo_start_synctest(&ggpo, &cb, "beatdown", 2, sizeof(int), 1);
#else
  result = ggpo_start_session(&ggpo, &cb, "beatdown", 2, sizeof(int), localPort);
#endif

  ggpo_set_disconnect_timeout(ggpo, 3000);
  ggpo_set_disconnect_notify_start(ggpo, 2000);

  // Add Player 1
  result = ggpo_add_player(ggpo, &p1, &player_handles[0]);
  result = ggpo_add_player(ggpo, &p2, &player_handles[1]);
}
