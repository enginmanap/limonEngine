//
// Created by engin on 31.12.2018.
//

#ifndef LIMONENGINE_COWBOYENEMYAI_H
#define LIMONENGINE_COWBOYENEMYAI_H


#include "API/ActorInterface.h"
#include <random>

/**
 * Cowboy Enemy can be doing one of these:
 * 1) Scripted loop
 * 2) idling until player detection
 * 3) walking to player if player is close
 * 4) hitting player with melee if player is hitting close
 * 5) running to player if player is at medium distance
 * 6) Shooting player
 * 7) kneeling to shoot
 * 8) kneel shooting
 * 9) standing up
 *
 * kneeling is to be selected by random, only if player is medium distance and visible
 */

class CowboyEnemyAI : public ActorInterface {

    enum class State {
        DEAD,
        IDLE,
        WALKING,
        RUNNING,
        SHOOTING,
        MELEE,
        KNEELING_DOWN,
        STANDING_UP,
        KNEEL_SHOOTING,
        KNEEL_IDLE,
        HIT,
        SCRIPTED
    };

    enum class Gun {
        PISTOL,
        RIFLE,
        SHOTGUN
    };

    State currentState = State::IDLE;
    State transitionState = State::IDLE;
    State beforeState = State::IDLE;//Shooting and Hit uses to return

    Gun transitionGun = Gun::RIFLE;

    std::string currentAnimationName;

    std::vector<glm::vec3> routeToRequest;

    long routeGetTime = 0;
    bool currentAnimationFinished;

    const float DETECTION_DISTANCE = 100;//too little because of testing
    const float MELEE_DISTANCE = 6;
    const float RUN_DISTANCE = 25;

    long lastSetupTime;
    long lastShootTime = 0;
    int shootingStage = 0;
    glm::vec3 lastWalkDirection = glm::vec3(0,0,0);
    bool routeRequested = false;

    float walkSpeed = 0.065;
    float runSpeed = 0.14;
    /********* Parameters to expose as setting *************/
    float shootChance = 0.85f;
    float kneelDownChance = 0.005f;
    float kneelStayChance = 0.0025f;
    long minShootTimeWait = 1000;
    uint32_t hitPoints = 100;
    Gun currentGun = Gun::RIFLE;
    uint32_t gunDamage = 15;

    /********* Parameters to expose as setting *************/
    std::uniform_real_distribution<float> randomFloats;
    std::default_random_engine generator;
    ActorInterface::ActorInformation latestInformation;
public:
    CowboyEnemyAI(uint32_t id, LimonAPI *limonAPI) : ActorInterface(id, limonAPI) {
        randomFloats = std::uniform_real_distribution<float>(0.0f, 1.0f); // generates random floats between 0.0 and 1.0
    };

    void play(long time, ActorInterface::ActorInformation &information) override;

    bool interaction(std::vector<LimonTypes::GenericParameter> &interactionInformation) override;

    std::vector<LimonTypes::GenericParameter> getParameters() const override;

    void setParameters(std::vector<LimonTypes::GenericParameter> parameters) override;


    std::string getName() const override {
        return "Cowboy Enemy";
    }

    void transitionToMelee(const ActorInformation &information);

    void transitionToWalk(const ActorInformation &information);

    void transitionToRun(const ActorInformation &information);

    void transitionToKneel(const ActorInformation &information);

    void transitionToKneelIdle(const ActorInformation &information);

    void transitionToKneelShoot(const ActorInformation &information);

    void transitionToStandUp(const ActorInformation &information);

    void transitionToShoot(const ActorInformation &information);

    void transitionToIdle(const ActorInformation &information);

    void turnFaceToPlayer(const ActorInformation &information);

    void transitionToHit();

    void transitionToDead();

    void shootPlayer(float playerDistance);

    void playShootSound(Gun gunType);
};


extern "C" void registerActors(std::map<std::string, ActorInterface*(*)(uint32_t, LimonAPI*)>*);



#endif //LIMONENGINE_COWBOYENEMYAI_H
