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

struct ThrowResult {
  bool thrown;
  CollisionBox* throwCb;
};

class Physics {
public:
  // TODO: Vectorize this
  Physics();
  ~Physics();

  HitResult checkHitbox(Character* hitter, Character* hurter);
  HitResult checkEntityHitbox(Character* hitter, Character* hurter);
  ThrowResult checkThrowbox(Character* thrower, Character* throwee);

  void checkProjectileBox(Character* hitter, Character* hurter);
  void checkProximityBox(Character* hitter, Character* hurter);
  void checkPushBox(Character* player1, Character* player2, int worldWidth);

  void checkCorner(Character* p1, int worldWidth);
  void checkBounds(Character* p1, Character* p2, Camera camera, int worldWidth);

private:
};
