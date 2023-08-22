#include "CollisionBox.h"
#include "Camera.h"
#include "Util.h"
// TODO: STOP BEING LAZY AND POLY THIS

CollisionBox::CollisionBox(){}
CollisionBox::CollisionBox(CollisionBox::CollisionType boxType, int width, int height, 
  int offsetX, int offsetY, int start, int end) 
: CollisionBox(boxType, width, height, offsetX, offsetY, start, end, 0,0,0,0,0,0,0) {}

CollisionBox::CollisionBox(CollisionBox::CollisionType boxType, int width, int height, 
  int offsetX, int offsetY, int start, int end, int damage, int pushback, int hitstop, int hitstun, int pushTime, int blockstun, int blockType)
: width(width), height(height), offsetX(offsetX), offsetY(offsetY), start(start), end(end), 
  damage(damage),pushback(pushback), pushTime(pushTime), hitstop(hitstop), hitstun(hitstun), blockstun(blockstun), blockType(blockType), boxType(boxType){ }

CollisionBox::~CollisionBox(){ }

CollisionRect CollisionBox::getAABBIntersect(CollisionBox box1, CollisionBox box2){
  std::pair<int, int> b1Pos(box1.positionX, box1.positionY);
  std::pair<int, int> b2Pos(box2.positionX, box2.positionY);

  int highestX = b1Pos.first < b2Pos.first? b2Pos.first: b1Pos.first;
  int lowestEdge = (b1Pos.first + box1.width) < (b2Pos.first + box2.width) ? (b1Pos.first + box1.width) : (b2Pos.first + box2.width);
  int highestY = b1Pos.second < b2Pos.second ? box1.positionY : box2.positionY;
  int lowestTop = (b1Pos.second - box1.height) > (b2Pos.second - box2.height) ? (b1Pos.second - box1.height) : (b2Pos.second - box2.height);

  int newX = highestX;
  int newW = (lowestEdge) - newX;
  int newY = highestY;
  int newH = abs(lowestTop - newY);
  // printf("newY:%d, newH:%d\n", newY, newH);
  return CollisionRect{newX, newY, newW, newH};
}

bool CollisionBox::checkAABB(CollisionBox box1, CollisionBox box2){
  if(box1.positionX < box2.positionX + box2.width &&
     box1.positionX + box1.width > box2.positionX &&
     box1.positionY < box2.positionY + box2.height &&
     box1.positionY + box1.height > box2.positionY) {

     return true;
  }


  return false;
};


CollisionBoxState CollisionBox::saveState(){
  stateObj.positionX = positionX;
  stateObj.positionY = positionY;
  stateObj.disabled = disabled;

  return stateObj;
}

void CollisionBox::loadState(CollisionBoxState _stateObj){
  positionX = _stateObj.positionX;
  positionY = _stateObj.positionY;
  disabled = _stateObj.disabled;
}


// void CollisionBox::render(){
//   Camera* cam = graphics->getCamera();
//   int windowHeight = graphics->getWindowHeight();
//   switch (boxType) {
//     case POSITION:
//       // red
//       SDL_SetRenderDrawColor(graphics->getRenderer(), 0, 0, 255, 0);
//       break;
//     case HURT:
//       // green
//       SDL_SetRenderDrawColor(graphics->getRenderer(), 0, 255, 0, 0);
//       break;
//     case HIT:
//       // blue
//       SDL_SetRenderDrawColor(graphics->getRenderer(), 255, 0, 0, 0);
//       break;
//     case THROW:
//       // bright purple 
//       SDL_SetRenderDrawColor(graphics->getRenderer(), 255, 0, 255, 0);
//       break;
//     case THROW_HURT:
//       // purple
//       SDL_SetRenderDrawColor(graphics->getRenderer(), 128, 0, 128, 0);
//       break;
//     case PROXIMITY:
//       // yellow
//       SDL_SetRenderDrawColor(graphics->getRenderer(), 255, 255, 0, 0);
//       break;
//     case PROJECTILE:
//       // yellow
//       SDL_SetRenderDrawColor(graphics->getRenderer(), 255, 255, 0, 0);
//       break;
//   }
//   SDL_Rect collisionRect;
// 
//   collisionRect.x = ((positionX/COORDINATE_SCALE) - cam->cameraRect.x);
//   collisionRect.y = ((positionY/COORDINATE_SCALE) + (windowHeight - (height/COORDINATE_SCALE)) - 60) + cam->cameraRect.y;
//   collisionRect.w = width/COORDINATE_SCALE;
//   collisionRect.h = height/COORDINATE_SCALE;
// 
//   SDL_RenderDrawRect(graphics->getRenderer(), &collisionRect);
//   SDL_SetRenderDrawColor(graphics->getRenderer(), 0xFF, 0xFF, 0xFF, 0xFF);
// }


std::map<std::string, CollisionBox::CollisionType> CollisionBox::collisionTypeMap = {
  {"POSITION", CollisionBox::POSITION},
  {"HURT", CollisionBox::HURT},
  {"HIT", CollisionBox::HIT},
  {"THROW", CollisionBox::THROW},
  {"THROW_HURT", CollisionBox::THROW_HURT},
  {"PROXIMITY", CollisionBox::PROXIMITY},
  {"PROJECTILE", CollisionBox::PROJECTILE},
};
