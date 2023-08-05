#ifndef _FIGHTINGGAMESERVER_H
#define _FIGHTINGGAMESERVER_H

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include "ggponet.h"
#include "VirtualController.h"
#include "Character.h"
#include "CollisionBox.h"
#include "Util.h"
#include "Camera.h"

struct GameState {
  int roundStartCounter;
  int roundStart;
  int currentRound;
  int inSlowDown;
  int p1RoundsWon;
  int p2RoundsWon;
  int roundEnd;
  int roundWinner;
  int screenFreeze;
  int screenFreezeCounter;
  int screenFreezeLength;
  int slowDownCounter;
  int slowMode;
  int frameCount;

  bool shouldUpdate;
  bool netPlayState;
  bool doneSync;

  CharStateObj player1;
  CharStateObj player2;
  CameraStateObj cameraState;
};

struct ThrowResult {
  bool thrown;
  CollisionBox* throwCb;
};

struct HitResult {
  bool hit;
  bool counter;
  int hitState;
  CollisionBox* hitCb;
};

struct TriggerResult {
  bool triggered;
  CollisionBox* triggerCb;
};


class FightingGameServer : public godot::Node{
  GDCLASS(FightingGameServer, Node)

  public:

    FightingGameServer ();
    ~FightingGameServer ();

    void enter();
    void step(int inputs[]);

    godot::Dictionary getGameState();
    godot::String getModelName(int p_charNum);
    double getModelScale(int p_charNum);

    void _physics_process(double_t delta) override;
    void _process(double_t delta) override;
    void _ready() override;

    // GGPO
    int getPort();
    std::string getIp();

    bool shouldUpdate = true;
    bool stepOnce = false;
    bool netPlayState, 
         doneSync,
         slowMode,
         everythingCompiled,
         inSlowDown,
         roundEnd,
         roundStart,
         screenFreeze = false;

    int frameCount,
        currentState,
        screenFreezeLength,
        slowDownCounter,
        roundWinner,
        screenFreezeCounter,
        currentRound,
        roundStartCounter,
        p1RoundsWon,
        p2RoundsWon,
        time_passed,
        localBufferSize,
        remotePort,
        localPort,
        netPnum = 0;

    std::string remoteIp, 
                localIp = "";

    int p1StartPos = 1700 * COORDINATE_SCALE;
    int p2StartPos = 2200 * COORDINATE_SCALE;

    int worldWidth = 3840 * COORDINATE_SCALE;
    int camWidth = 1280 * COORDINATE_SCALE;
    int camHeight = 720 * COORDINATE_SCALE;

    unsigned char* localBuffer = 0;
    Camera camera;
    VirtualController p1Vc;
    VirtualController p2Vc;
    Character player1 = Character(std::make_pair(p1StartPos, 0), 1);
    Character player2 = Character(std::make_pair(p2StartPos, 0), 2);

  protected:
    static void _bind_methods();

  private:
    int readGodotInputs(int pNum);
    void readGodotTrainingInput();

    void checkTriggerCollisions();
    void checkPushCollisions();
    void checkThrowCollisions();
    void checkHitCollisions();
    void checkProximityCollisions();
    void checkEntityHitCollisions();
    void checkBounds();
    void checkHealth();
    bool checkBlock(int blockType, Character* player);
    void updateFaceRight();
    void handleRoundStart();
    void restartRound();
    void checkThrowTechs();
    void updateVisuals();
    void updateCamera();

    void checkCorner(Character* player);
    void checkHitstop(Character* player);
    void checkEntityHitstop(Character* player);

    HitResult checkHitboxAgainstHurtbox(Character* hitter, Character* hurter);
    int checkProximityAgainst(Character* hitter, Character* hurter);

    HitResult checkEntityHitAgainst(Character* thrower, Character* throwee);
    ThrowResult checkThrowAgainst(Character* thrower, Character* throwee);
    TriggerResult checkTriggerAgainst(Character* owner, Character* activator);
    void handleSameFrameThrowTech(SpecialState techState);
    int checkProjectileCollisions(Character* player1, Character* player2);

    //Training mode functions
    void saveState();
    void loadState();
    void pauseState();
    void unpauseState();
    void stepState();

    // GGPO
    void ggpoInit(); 
};

#endif
