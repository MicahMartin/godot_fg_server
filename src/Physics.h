#include "Character.h"
#include "CollisionBox.h"
#include "Util.h"
#include "Camera.h"

struct HitResult {
  bool hit;
  bool counter;
  int hitState;
  CollisionBox* hitCb;
};

class Physics {
public:
  Physics();
  ~Physics();

  HitResult checkHitboxAgainstHurtbox(Character* hitter, Character* hurter);
  void checkCorner(Character* p1, int worldWidth);
  void checkBounds(Character* p1, Character* p2, Camera camera, int worldWidth);



private:
};
