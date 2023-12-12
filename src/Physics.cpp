#include "Physics.h"

Physics::Physics() {
}

Physics::~Physics() {
}


bool isBlocking(bool blockState, int blockType, Character* character){
  return (
    blockState && blockType == 1)
    || (blockState && character->checkBlock(blockType))
    || (character->control && character->checkBlock(blockType)
  );
}

void handleBlock(Character* hurter, Character* hitter, CollisionBox& hitBox){
  bool instantBlocked = hurter->_checkCommand(11);
  int realBlockstun = hitBox.blockstun;
  int realPushback = hitBox.pushback;

  // Instant block
  if (instantBlocked) {
    realBlockstun -= 4;
    realBlockstun = realBlockstun <= 0 ? 1 : realBlockstun;
    hurter->blockstun = realBlockstun;
  }
  else {
    hurter->blockstun = realBlockstun;
  }

  // Push Block
  if (hurter->pushBlockState(hurter->currentState->stateNum)){
    hurter->blockstun = realBlockstun + 4;
    realPushback *= 2;
  }

  hurter->control = 0;
  bool holdingButtons = hurter->_getInput(11) && hurter->_getInput(12);
  bool downBack = hurter->_getInput(1);
  bool backOrUpback = (hurter->_getInput(4) || hurter->_getInput(7)) && !downBack;
  bool anyBack = (hurter->_getInput(1) || hurter->_getInput(4) || hurter->_getInput(7));

  bool crouchPB = holdingButtons && downBack;
  bool standPB = holdingButtons && backOrUpback;

  // Air Block
  if (hurter->_getYPos() > 0) {
    if (anyBack && holdingButtons) {
      hurter->changeState(hurter->specialStateMap[SS_AIR_PUSH_BLOCK]);
    }
    else {
      hurter->changeState(hurter->specialStateMap[SS_AIR_BLOCK]);
    }
  }

  else {
    switch (hitBox.blockType) {
      case 1:
        if (hurter->_getInput(1)) {
          if (crouchPB) {
            // Mix_PlayChannel(0, pushBlock, 0);
            hurter->changeState(hurter->specialStateMap[SS_CROUCH_PUSH_BLOCK]);
            // hurter->_subtractMeter(1020);
          }
          else {
            hurter->changeState(hurter->specialStateMap[SS_BLOCK_CROUCH]);
          }
        }
        else {
          if (standPB) {
            // hurter->_subtractMeter(1020);
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
          // hurter->_subtractMeter(1020);
          hurter->changeState(hurter->specialStateMap[SS_CROUCH_PUSH_BLOCK]);
        }
        else {
          hurter->changeState(hurter->specialStateMap[SS_BLOCK_CROUCH]);
        }
        break;
      case 3:
        if (standPB) {
          // Mix_PlayChannel(0, pushBlock, 0);
          // hurter->_subtractMeter(1020);
          hurter->changeState(hurter->specialStateMap[SS_PUSH_BLOCK]);
        }
        hurter->changeState(hurter->specialStateMap[SS_BLOCK_STAND]);
        break;
        // should throw error here?
      default: break;
    }
  }

  if (hurter->inCorner && (hitBox.pushback > 0)) {
    hitter->pushTime = hitBox.blockstun;
    if (hitter->faceRight) {
      hitter->pushBackVelocity = realPushback;
    }
    else {
      hitter->pushBackVelocity = -(realPushback);
    }
  } else {
    hurter->pushTime = hitBox.pushTime;
    if (hitter->faceRight) {
      hurter->pushBackVelocity = -realPushback;
    }
    else {
      hurter->pushBackVelocity = realPushback;
    }
  }
}

void handleHitStop(Character* hitter, Character* hurter, CollisionBox& hitBox) {
  hitter->inHitStop = true;
  hitter->hitStop = hitBox.hitstop;

  hurter->hitStop = hitBox.hitstop;
  hurter->inHitStop = true;
  // hitter->frameLastAttackConnected = frameCount;
  hitter->currentState->hitboxGroupDisabled[hitBox.groupID] = true;
  hitter->currentState->canHitCancel = true;
}


HitResult handleHit(Character* hitter, Character* hurter, CollisionBox& hitBox) {
  int finalHitPush = 1;
  StateDef* hitterState = hitter->currentState;
  StateDef* hurterState = hurter->currentState;

  if (hurter->timeInHitstun > 600) {
    finalHitPush = 3;
    hurter->hurtGravity = hurter->gravityVal * 3;
  }
  else if (hurter->timeInHitstun > 300) {
    finalHitPush = 2;
    hurter->hurtGravity = hurter->gravityVal * 2;
  }

  // Push back
  if (hurter->inCorner) {
    hitter->pushTime = hitBox.hitPushTime;
    if (hitter->faceRight) {
      hitter->pushBackVelocity = hitBox.hitVelocityX * finalHitPush;
    }
    else {
      hitter->pushBackVelocity = -(hitBox.hitVelocityX * finalHitPush);
    }
  }
  else {
    hurter->hitPushTime = hitBox.hitPushTime;

    if (hitter->faceRight) {
      hurter->hitPushVelX = -(hitBox.hitVelocityX * finalHitPush);
    }
    else {
      hurter->hitPushVelX = hitBox.hitVelocityX * finalHitPush;
    }

  }

  hurter->comboCounter++;
  bool wasACounter = hurterState->counterHitFlag;
  hurterState->counterHitFlag = false;

  if (wasACounter) {
    hurter->isRed = true;
  }

  if (hurter->comboCounter == 1) {
    hurter->comboProration = hitBox.initialProration;
  }

  hurter->control = 0;
  int finalHitstun = wasACounter ? (hitBox.hitstun + 4) : (hitBox.hitstun);
  finalHitstun += (hurter->_getYPos() > 0) ? 4 : 0;

  if (finalHitstun < 1) {
    finalHitstun = 1;
  }

  int finalDamage = hitBox.damage;
  finalDamage = ((finalDamage * hurter->defenseValue) / 100);
  hurter->comboDamage += finalDamage;
  hurter->health -= finalDamage;
  hurter->hitstun = finalHitstun;

  if ((hitBox.hitType == LAUNCHER)
      || hitBox.hitType == FLOATER
      || hitBox.hitType == GROUND_BOUNCE
      || hurter->_getYPos() > 0
      || hurter->airHurtState(hurterState->stateNum)) {
    if (hitBox.airHitstun > 0) {
      hurter->hitstun = hitBox.airHitstun;
    }
    if (hitBox.airHitVelocityX > 0) {
      if (hurter->inCorner && (hitBox.pushback > 0)) {
        hitter->pushTime = hitBox.hitPushTime;
        if (hitter->faceRight) {
          hitter->pushBackVelocity = hitBox.airHitVelocityX / 2;
        }
        else {
          hitter->pushBackVelocity = -(hitBox.airHitVelocityX / 2);
        }
      }
      else {
        hurter->hitPushTime = hitBox.airHitPushTime > 0 ? 
          hitBox.airHitPushTime 
          : hitBox.hitPushTime;

        if (hitter->faceRight) {
          hurter->hitPushVelX = -hitBox.airHitVelocityX;
        }
        else {
          hurter->hitPushVelX = hitBox.airHitVelocityX;
        }

      }
    }

    hurter->velocityY = hitBox.hitVelocityY;
    SpecialState hurtState;
    switch (hitBox.hitType) {
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


void handleHitSpark(Character* hitter, CollisionRect hitsparkIntersect, VisualEffect* hitSpark) {
  int xEdge = hitter->faceRight ? 
    hitsparkIntersect.x + hitsparkIntersect.w 
    : hitsparkIntersect.x;
  // int visualID = hitBox.hitsparkID;
  hitSpark->reset(xEdge, (hitsparkIntersect.y + (hitsparkIntersect.h / 2)));
  hitSpark->setActive(true);
}


HitResult Physics::checkHitbox(Character* hitter, Character* hurter) {
  StateDef* hitterState = hitter->currentState;
  StateDef* hurterState = hurter->currentState;

  for (auto hId : hitterState->hitBoxIds) {

    CollisionBox& hitBox = hitter->getCollisionBox(hId);
    bool hitGroupDisabled = hitterState->hitboxGroupDisabled[hitBox.groupID];
    int blockType = hitBox.blockType;

    if (!hitBox.disabled && !hitGroupDisabled) {
      for (auto uId : hurterState->hurtBoxIds) {

        CollisionBox& hurtBox = hurter->getCollisionBox(uId);
        bool hurtGroupDisabled = hurterState->hitboxGroupDisabled[hurtBox.groupID];

        if (!hurtBox.disabled && !hurtGroupDisabled) {
          if (CollisionBox::checkAABB(hitBox, hurtBox)) {
            //TODO: SHAKING SCRIPT
            // shakeCamera(hitBox.hitstop, &camera);
            CollisionRect hitsparkIntersect = CollisionBox::getAABBIntersect(hitBox, hurtBox);

            int hurterStateNum = hurterState->stateNum;
            int hitterStateNum = hitterState->stateNum;
            bool blockState = hurter->blockState(hurterStateNum);

            handleHitStop(hitter, hurter, hitBox);

            if (isBlocking(blockState, blockType, hurter)) {
              handleBlock(hurter, hitter, hitBox);
              handleHitSpark(hitter, hitsparkIntersect, &hitter->hitSpark);
            }
            else {
              HitResult result = handleHit(hitter, hurter, hitBox);
              handleHitSpark(hitter, hitsparkIntersect, &hitter->hitSpark);
              return result;
            }
          }
        }
      }
    }
  }
  return { false, false, 0, NULL };
}

HitResult Physics::checkEntityHitbox(Character* hitter, Character* hurter) {
  for (auto& entity : hitter->entityList) {
    if (entity.active && !entity.currentState->hitboxesDisabled) {
      for (auto eId : entity.currentState->projectileBoxIds) {
        CollisionBox& entityHitBox = entity.getCollisionBox(eId);

        bool groupDisabled = entity.currentState->hitboxGroupDisabled[entityHitBox.groupID];
        if (!entityHitBox.disabled && !groupDisabled) {
          for (auto hId : hurter->currentState->hurtBoxIds) {
            CollisionBox& hurtBox = hurter->getCollisionBox(hId);

            if (!hurtBox.disabled && !entity.inHitStop) {
              if (CollisionBox::checkAABB(entityHitBox, hurtBox)) {
                bool entityFaceRight = entity.faceRight;
                entity.inHitStop = true;
                if (entityHitBox.selfHitstop > 0) {
                  entity.hitStop = entityHitBox.selfHitstop;
                }
                else {
                  entity.hitStop = entityHitBox.hitstop;
                }

                hurter->inHitStop = true;
                hurter->hitStop = entityHitBox.hitstop;

                godot::UtilityFunctions::print("entity durability:", entity.currentDurability);
                if (--entity.currentDurability <= 0) {
                  entity.currentState->hitboxGroupDisabled[entityHitBox.groupID] = true;
                  entity.currentState->canHitCancel = true;
                };

                int p2StateNum = hurter->currentState->stateNum;

                if ((hurter->blockState(p2StateNum)) || (hurter->control && hurter->checkBlock(entityHitBox.blockType))) {
                  hurter->control = 0;
                  bool instantBlocked = hurter->_checkCommand(11);
                  if (instantBlocked) {
                    hurter->isLight = true;
                    // Mix_PlayChannel(0, instantBlock, 0);
                    hurter->tensionGained += 100;
                    int realBlockstun = entityHitBox.blockstun - 4;
                    realBlockstun = realBlockstun <= 0 ? 1 : realBlockstun;
                    hurter->blockstun = realBlockstun;
                  }
                  else {
                    hurter->blockstun = entityHitBox.blockstun;
                  }
                  if (hurter->_getYPos() > 0) {
                    // TODO: air blocking state
                    hurter->changeState(hurter->specialStateMap[SS_AIR_BLOCK]);
                  }
                  else {
                    switch (entityHitBox.blockType) {
                      case 1:
                        if (hurter->_getInput(1)) {
                          hurter->changeState(hurter->specialStateMap[SS_BLOCK_CROUCH]);
                        }
                        else {
                          hurter->changeState(hurter->specialStateMap[SS_BLOCK_STAND]);
                        }
                        break;
                      case 2:
                        hurter->changeState(hurter->specialStateMap[SS_BLOCK_CROUCH]);
                        break;
                      case 3:
                        hurter->changeState(hurter->specialStateMap[SS_BLOCK_STAND]);
                        break;
                        // should throw error here
                      default: break;
                    }
                  }

                  hurter->pushTime = entityHitBox.pushTime;
                  if (hurter->faceRight == entityFaceRight) {
                    if (hurter->faceRight) {
                      hurter->pushBackVelocity = -entityHitBox.pushback;
                    }
                    else {
                      hurter->pushBackVelocity = entityHitBox.pushback;
                    }
                  }
                  else {
                    if (hurter->faceRight) {
                      hurter->pushBackVelocity = entityHitBox.pushback;
                    }
                    else {
                      hurter->pushBackVelocity = -entityHitBox.pushback;
                    }
                  }
                  printf("got to the entity visfx\n");
                }
                else {

                  printf("bug to the entity here\n");
                  hurter->hitPushTime = entityHitBox.hitPushTime;
                  if (hurter->faceRight == entityFaceRight) {
                    if (hurter->faceRight) {
                      hurter->hitPushVelX = -entityHitBox.hitVelocityX;
                    }
                    else {
                      hurter->hitPushVelX = entityHitBox.hitVelocityX;
                    }
                  }
                  else {
                    if (hurter->faceRight) {
                      hurter->hitPushVelX = entityHitBox.hitVelocityX;
                    }
                    else {
                      hurter->hitPushVelX = -entityHitBox.hitVelocityX;
                    }
                  }
                  bool wasACounter = hurter->currentState->counterHitFlag;
                  hurter->currentState->counterHitFlag = false;
                  if (wasACounter) {
                  }

                  printf("got to the entity hit visfx\n");
                  printf("got to the entity hurt sound\n");

                  hurter->control = 0;
                  hurter->health -= entityHitBox.damage;
                  hurter->hitstun = entityHitBox.hitstun;
                  hurter->comboCounter++;
                  if (entityHitBox.hitType == LAUNCHER
                      || hurter->_getYPos() > 0
                      || hurter->currentState->stateNum == hurter->specialStateMap[SS_AIR_HURT]) {
                    hurter->velocityY = entityHitBox.hitVelocityY;
                    printf("got to the launcher return");
                    return { true, false, hurter->specialStateMap[SS_AIR_HURT], NULL };
                  }
                  else {

                    printf("got to the normal return\n");
                    return { true, false, hurter->specialStateMap[SS_HURT], NULL };
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

ThrowResult Physics::checkThrowbox(Character* thrower, Character* throwee) {
  ThrowResult throwResult = ThrowResult{ false, NULL };
  for (auto p1Id : thrower->currentState->throwHitBoxIds) {
    if (!thrower->getCollisionBox(p1Id).disabled) {
      for (auto p2Id : throwee->currentState->throwHurtBoxIds) {
        if (!throwee->getCollisionBox(p2Id).disabled) {
          if (CollisionBox::checkAABB(thrower->getCollisionBox(p1Id), throwee->getCollisionBox(p2Id))) {
            if (thrower->getCollisionBox(p1Id).throwType == 1 && throwee->_getYPos() > 0) {
              throwResult.thrown = true;
              throwResult.throwCb = &thrower->getCollisionBox(p1Id);
              thrower->currentState->hitboxesDisabled = true;
            }
            else if (thrower->getCollisionBox(p1Id).throwType == 2 && throwee->_getYPos() == 0) {
              godot::UtilityFunctions::print("frame: ", thrower->currentState->stateTime," throws, conrol:", thrower->control);
              throwResult.thrown = true;
              throwResult.throwCb = &thrower->getCollisionBox(p1Id);
              thrower->currentState->hitboxesDisabled = true;
            }
          }
        }
      }
    }
  }
  return throwResult;
}

void Physics::checkProjectileBox(Character* hitter, Character* hurter){
  for (auto& entity : hitter->entityList) {
    if (entity.active && !entity.currentState->hitboxesDisabled) {
      for (auto eId : entity.currentState->projectileBoxIds) {
        CollisionBox& hitboxRef = entity.getCollisionBox(eId);
        bool groupDisabled = entity.currentState->hitboxGroupDisabled[hitboxRef.groupID];
        if (!groupDisabled && !entity.inHitStop) {
          for (auto& otherEntity : hurter->entityList) {
            if (otherEntity.active && !otherEntity.currentState->hitboxesDisabled) {
              for (auto oId : otherEntity.currentState->projectileBoxIds) {
                CollisionBox& hurtboxRef = otherEntity.getCollisionBox(oId);

                bool otherGroupDisabled = otherEntity.currentState->hitboxGroupDisabled[hurtboxRef.groupID];
                if (!otherGroupDisabled && !otherEntity.inHitStop) {
                  if (CollisionBox::checkAABB(hitboxRef, hurtboxRef)) {
                    entity.inHitStop = true;
                    entity.hitStop = 6;
                    otherEntity.inHitStop = true;
                    otherEntity.hitStop = 6;
                    if (--entity.currentDurability <= 0) {

                      entity.currentState->hitboxGroupDisabled[hitboxRef.groupID] = true;
                      entity.currentState->canHitCancel = true;
                    };
                    if (--otherEntity.currentDurability <= 0) {
                      otherEntity.currentState->hitboxGroupDisabled[hurtboxRef.groupID] = true;
                      otherEntity.currentState->canHitCancel = true;
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
}

void Physics::checkProximityBox(Character* p1, Character* p2){
  if (p2->getPos().second <= 0) {
    if (!p1->currentState->hitboxesDisabled) {
      for (auto hitBoxId : p1->currentState->proximityBoxIds) {
        CollisionBox& pushBox1 = p1->getCollisionBox(hitBoxId);
        bool groupDisabled = p1->currentState->hitboxGroupDisabled[pushBox1.groupID];

        if (!pushBox1.disabled && !groupDisabled) {
          for (auto hurtBoxId : p2->currentState->hurtBoxIds) {
            CollisionBox& pushBox2 = p2->getCollisionBox(hurtBoxId);

            if (!pushBox2.disabled && !groupDisabled) {
              if (CollisionBox::checkAABB(pushBox1, pushBox2)) {
                if (p2->currentState->stateNum == p2->specialStateMap[SS_WALK_B]) {
                  p2->changeState(p2->specialStateMap[SS_BLOCK_STAND]);
                }
                if (p2->currentState->stateNum == p2->specialStateMap[SS_CROUCH] && p2->_getInput(1)) {
                  p2->changeState(p2->specialStateMap[SS_BLOCK_CROUCH]);
                }
              }
            }
          }
        }
      }
    }

    for (auto& entity : p1->entityList) {
      if (entity.active && !entity.currentState->hitboxesDisabled) {
        for (auto eId : entity.currentState->proximityBoxIds) {
          CollisionBox& pushBox1 = entity.getCollisionBox(eId);

          bool groupDisabled = entity.currentState->hitboxGroupDisabled[pushBox1.groupID];
          if (!pushBox1.disabled && !groupDisabled) {
            for (auto p2Id : p2->currentState->hurtBoxIds) {
              CollisionBox& pushBox2 = p2->getCollisionBox(p2Id);

              if (!pushBox2.disabled && !entity.inHitStop) {
                if (CollisionBox::checkAABB(pushBox1, pushBox2)) {
                  if (p2->currentState->stateNum == p2->specialStateMap[SS_WALK_B]) {
                    p2->changeState(p2->specialStateMap[SS_BLOCK_STAND]);
                  }
                  if (p2->currentState->stateNum == p2->specialStateMap[SS_CROUCH] && p2->_getInput(1)) {
                    p2->changeState(p2->specialStateMap[SS_BLOCK_CROUCH]);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

void Physics::checkPushBox(Character* player1, Character* player2, int worldWidth){
  // get the collision box(s) for the current state
  std::pair<int, int> p1Pos = player1->getPos();
  std::pair<int, int> p2Pos = player2->getPos();

  for (auto& p1PbId : player1->currentState->pushBoxIds) {
    if (!player1->getCollisionBox(p1PbId).disabled) {
      CollisionBox& pushBox1 = player1->getCollisionBox(p1PbId);

      for (auto p2PbId : player2->currentState->pushBoxIds) {
        CollisionBox& pushBox2 = player2->getCollisionBox(p2PbId);

        if (!pushBox2.disabled) {
          if (CollisionBox::checkAABB(pushBox1, pushBox2)) {
            // find how deeply intersected they are
            bool p1Lefter = p1Pos.first < p2Pos.first;
            if (p1Pos.first == p2Pos.first) {
              p1Lefter = player1->faceRight;
            }

            if (p1Lefter) {
              int p1RightEdge = pushBox1.positionX + pushBox1.width;
              int p2LeftEdge = pushBox2.positionX;
              int depth = p1RightEdge - p2LeftEdge;

              // account for over bound 
              if ((p2Pos.first + player2->width) + (depth / 2) > worldWidth) {
                // int remainder = worldWidth - (p2Pos.first + (depth / 2));
                player2->setXPos(worldWidth - player2->width);
                player1->setX(-depth);
              }
              else if ((p1Pos.first - player1->width) - (depth / 2) < 0) {
                player1->setXPos(0 + player1->width);
                player2->setX(depth);
              }
              else {
                player2->setX(depth / 2);
                player1->setX(-depth / 2);
              }
            }
            else {
              int p2RightEdge = pushBox2.positionX + pushBox2.width;
              int p1LeftEdge = pushBox1.positionX;
              int depth = p2RightEdge - p1LeftEdge;

              // account for over bound 
              if ((p1Pos.first + player1->width) + (depth / 2) > worldWidth) {
                player1->setXPos(worldWidth + player1->width);
                player2->setX(-depth);
              }
              else if ((p2Pos.first - player2->width) - (depth / 2) < 0) {
                player2->setXPos(0 + player2->width);
                player1->setX(depth);
              }
              else {
                player2->setX(-depth / 2);
                player1->setX(depth / 2);
              }
            }

            player1->updateCollisionBoxPositions();
            player2->updateCollisionBoxPositions();
          }
        }
      }
    }
  }
}

void Physics::checkCorner(Character* p1, int worldWidth){
  if (p1->getPos().first - p1->width <= 0 || p1->getPos().first + p1->width >= worldWidth) {
    p1->inCorner = true;
  }
  else {
    p1->inCorner = false;
  }
}
void Physics::checkBounds(Character* p1, Character* p2, Camera camera, int worldWidth){

  if (p1->getPos().first - p1->width < 0) {
    p1->setXPos(0 + p1->width);
    p1->updateCollisionBoxPositions();
  }
  if (p1->getPos().first - p1->width < camera.lowerBound) {
    p1->setXPos(camera.lowerBound + p1->width);
    p1->updateCollisionBoxPositions();
  }

  if (p1->getPos().first + p1->width > worldWidth) {
    p1->setXPos(worldWidth - p1->width);
    p1->updateCollisionBoxPositions();
  }
  if (p1->getPos().first + p1->width > camera.upperBound) {
    p1->setXPos(camera.upperBound - p1->width);
    p1->updateCollisionBoxPositions();
  }

  for (auto& entity : p1->entityList) {
    if (entity.active && entity.isFireball) {
      int entityX = entity.getPos().first;
      bool lowerBound = (entityX) < 0;
      bool upperBound = (entityX) > worldWidth;

      if (lowerBound || upperBound) {
        entity.deactivateEntity();
      }
    }
  }

  if (p2->getPos().first - p2->width < 0) {
    p2->setXPos(0 + p2->width);
    p2->updateCollisionBoxPositions();
  }
  if (p2->getPos().first - p2->width < camera.lowerBound) {
    p2->setXPos(camera.lowerBound + p2->width);
    p2->updateCollisionBoxPositions();
  }

  if (p2->getPos().first + p2->width > worldWidth) {
    p2->setXPos(worldWidth - p2->width);
    p2->updateCollisionBoxPositions();
  }
  if (p2->getPos().first + p2->width > camera.upperBound) {
    p2->setXPos(camera.upperBound - p2->width);
    p2->updateCollisionBoxPositions();
  }
  for (auto& entity : p2->entityList) {
    if (entity.active && entity.isFireball) {
      int entityX = entity.getPos().first;
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
