//
// Created by engin on 26.12.2018.
//

#ifndef LIMONENGINE_COWBOYSHOOTEREXTENSION_H
#define LIMONENGINE_COWBOYSHOOTEREXTENSION_H

#include <iostream>
#include "API/PlayerExtensionInterface.h"
#include "glm/gtc/quaternion.hpp"
/**
 * Cowboy attachment of player can be doing one of these:
 * 1) idling
 * 2) walking
 * 3) running
 * 4) shooting
 */
class CowboyShooterExtension : public  PlayerExtensionInterface {
    enum class State {
        IDLE,
        WALKING,
        RUNNING,
        SHOOTING
    };

    enum class Gun {
        PISTOL,
        RIFLE
    };

    State currentState = State::IDLE;
    State transitionState = State::IDLE;

    Gun currentGun = Gun::PISTOL;
    Gun transitionGun = Gun::PISTOL;

    std::string currentAnimationName;
    bool currentAnimationFinished;

    const glm::quat direction   = glm::quat(0.0f, 0.0f, 1.0f, 0.0f);//this is used to reverse hit normal
    const glm::vec3 pistolMuzzleFlashOffset = glm::vec3(0.010f, 0.173f, 0.844f);
    const glm::vec3 rifleMuzzleFlashOffset = glm::vec3(-4.140f, 44.759f, 8.279f);
    glm::vec3 currentMuzzleFlashOffset = pistolMuzzleFlashOffset;
    uint32_t playerAttachedModelID;
    uint32_t playerAttachedPistolID;
    uint32_t playerAttachedRifleID;
    long lastInputTime = 0;
    long hitTime = 0;

    const LimonAPI::Vec4 removeOffset = LimonAPI::Vec4(0, 0, -50);
    const LimonAPI::Vec4 addOffset = LimonAPI::Vec4(0, 0, 50);
    PlayerExtensionInterface::PlayerInformation latestPlayerInformation;

    int hitPoints = 100;
    bool hitReaction = false;

    InputStates inputState;

    void shootingTransition();
    void walkingTransition();
    void runningTransition();
    void idleTransition();

    bool changeGuns();

public:

    CowboyShooterExtension(LimonAPI* limonAPI) : PlayerExtensionInterface(limonAPI) {
        playerAttachedModelID = limonAPI->getPlayerAttachedModel();
        std::vector<uint32_t > children = limonAPI->getModelChildren(playerAttachedModelID);
        if(children.size() == 0) {
            std::cerr << "player attachment has no child, it should have a gun!" << std::endl;
            playerAttachedPistolID = 0;
        } else {
            playerAttachedPistolID = children[0];
            if(children.size() > 1) {
                playerAttachedRifleID = children[1];
                currentGun = Gun::PISTOL;
            }
        }
    }
    void removeDamageIndicator(std::vector<LimonAPI::ParameterRequest> parameters);
    void removeMuzzleFlash(std::vector<LimonAPI::ParameterRequest> parameters);
    void processInput(const InputStates &inputState, const PlayerExtensionInterface::PlayerInformation &playerInformation,
                          long time) override;

    void interact(std::vector<LimonAPI::ParameterRequest> &interactionData) override;

    std::string getName() const override;

    void processHitReaction();
};


extern "C" void registerPlayerExtensions(std::map<std::string, PlayerExtensionInterface*(*)(LimonAPI*)>*);






#endif //LIMONENGINE_COWBOYSHOOTEREXTENSION_H
