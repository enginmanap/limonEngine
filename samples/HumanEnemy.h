//
// Created by engin on 27.11.2017.
//

#ifndef LIMONENGINE_HUMANENEMY_H
#define LIMONENGINE_HUMANENEMY_H


#include "limonAPI/ActorInterface.h"
#include "limonAPI/LimonConverter.h"

class HumanEnemy: public ActorInterface {

    const long PLAYER_SHOOT_TIMEOUT = 1000;
    uint32_t playerPursuitStartTime = 0;
    long playerPursuitTimeout = 500000L; //if not see player for this amount, return.
    bool returnToPositionAfterPursuit = false;
    glm::vec3 initialPosition;
    glm::vec3 lastWalkDirection;
    std::string currentAnimation;
    bool hitAnimationAwaiting = false;
    uint32_t dieAnimationStartTime = 0;
    uint32_t hitAnimationStartTime = 0;
    uint32_t lastSetupTime;
    uint32_t shootPlayerTimer = 0;
    uint32_t hitPoints = 100;

    std::vector<glm::vec3> routeToRequest;
    uint32_t routeGetTime = 0;
    bool routeRequested = false;

public:
    HumanEnemy(uint32_t id, LimonAPI *limonAPI) : ActorInterface(id, limonAPI) {
        lastWalkDirection = this->getPosition();
    }

    void play(uint32_t time, ActorInterface::ActorInformation &information) override;

    bool interaction(std::vector<LimonTypes::GenericParameter> &interactionInformation) override;

    std::vector<LimonTypes::GenericParameter> getParameters() const override;

    void setParameters(std::vector<LimonTypes::GenericParameter> parameters) override;


    std::string getName() const override {
        return "ENEMY_AI_SWAT";
    }
};


extern "C" void registerActors(std::map<std::string, ActorInterface*(*)(uint32_t, LimonAPI*)>*);



#endif //LIMONENGINE_HUMANENEMY_H
