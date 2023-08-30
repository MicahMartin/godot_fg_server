#ifndef _VisualEffect_h
#define _VisualEffect_h

#include <utility>

class VisualEffect {
public:
  VisualEffect();
  ~VisualEffect();

  void update();
  void draw();

  void draw(bool faceRight);
  void reset(int xPos, int yPos);

  std::pair<int,int> getPos();
  void setX(int x);
  void setY(int y);
  int getX();
  int getY();
  
  void setActive(bool active);
  bool getActive();

  void setPlayLength(int newPlayLength);
  int getPlayLength();

  void incrementStateTime();
  void setStateTime(int newStateTime);
  int getStateTime();

  void setAura(bool aura);
  bool getAura();

  bool charEffect = false;
private:
  bool aura = false;
  bool isActive = false;
  int stateTime = 0;
  int xPos = 0;
  int yPos = 0;
  int playLength = 0;
};

#endif
