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


void handleHitSpark(Character* hitter, CollisionRect hitsparkIntersect) {
  int xEdge = hitter->faceRight ? 
    hitsparkIntersect.x + hitsparkIntersect.w 
    : hitsparkIntersect.x;
  // int visualID = hitBox.hitsparkID;
  hitter->hitSpark.reset(xEdge, (hitsparkIntersect.y + (hitsparkIntersect.h / 2)));
  hitter->hitSpark.setActive(true);
}


HitResult Physics::checkHitboxAgainstHurtbox(Character* hitter, Character* hurter) {
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
            handleHitSpark(hitter, hitsparkIntersect);

            int hurterStateNum = hurterState->stateNum;
            int hitterStateNum = hitterState->stateNum;
            bool blockState = hurter->blockState(hurterStateNum);

            handleHitStop(hitter, hurter, hitBox);

            if (isBlocking(blockState, blockType, hurter)) {
              handleBlock(hurter, hitter, hitBox);
            }
            else {
              HitResult result = handleHit(hitter, hurter, hitBox);
              return result;
            }
          }
        }
      }
    }
  }
  return { false, false, 0, NULL };
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
