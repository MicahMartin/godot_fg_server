#ifndef _COLLISIONBOX_H
#define _COLLISIONBOX_H

#include <utility>
#include <string>
#include <map>


struct CollisionRect {
  int x, y, w, h;
};
enum HitType {
  NULL_HIT_TYPE,
  NORMAL,
  LAUNCHER,
  GROUND_BOUNCE,
  WALL_BOUNCE,
  FLOATER
};

struct CollisionBoxState {
  int positionX;
  int positionY;
  bool disabled; 
};

class CollisionBox {
public:

  enum CollisionType {
    POSITION,
    HURT,
    HIT,
    THROW,
    THROW_HURT,
    PROXIMITY,
    PROJECTILE,
  };

  enum MatchFlag {
    OPPONENT_HIT_LOW,
    OPPONENT_HIT_HIGH,
    OPPONENT_HIT_MID,
  };


  static bool checkAABB(CollisionBox box1, CollisionBox box2);
  static CollisionRect getAABBIntersect(CollisionBox box1, CollisionBox box2);
  static std::map<std::string, CollisionType> collisionTypeMap;

  CollisionBox(CollisionType boxType, int width, int height, int offsetX, 
      int offsetY, int start, int end);

  CollisionBox(CollisionType boxType, int width, int height, int offsetX, 
      int offsetY, int start, int end, int damage, int pushback, int hitstop, int hitstun, int pushTime, int blockstun, int blocktype);

  CollisionBox();
  ~CollisionBox();

  CollisionBoxState saveState();
  void loadState(CollisionBoxState stateObj);
  

  int positionX;
  int positionY;
  bool disabled = false; 

  int width;
  int height; 

  int offsetX; 
  int offsetY;

  int start;
  int end;
  int initialProration;

  // i know, poly
  int damage;
  int pushback;
  int pushTime;
  int hitstop;
  int hitstun;
  int airHitstun = 0;
  int blockstun;
  int blockType;

  int hitType = NORMAL;
  int hitVelocityX = 0;
  int hitVelocityY = 0;
  int hitPushTime = 0;
  int airHitPushTime = 0;
  int airHitVelocityX = 0;
  int hitMeterGain = 0;
  int riskLower = 1;
  int scaleLower = 1;
  int selfHitstop = 0;

  int throwType;
  int throwSuccess;
  int throwAttempt;
  int techAttempt;
  int opponentTechAttempt;
  int opponentThrowSuccess;

  int selfState;
  int activatorState;
  int collisionBoxId;

  int groupID = 1;
  int hitSoundID = 0;
  int guardSoundID = 0;
  int hitsparkID = 0;
  int guardsparkID = 0;
  bool canTrip = false;

  CollisionBoxState stateObj;
  CollisionType boxType;
private:
};

#endif
