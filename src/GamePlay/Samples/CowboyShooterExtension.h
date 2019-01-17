//
// Created by engin on 26.12.2018.
//

#ifndef LIMONENGINE_COWBOYSHOOTEREXTENSION_H
#define LIMONENGINE_COWBOYSHOOTEREXTENSION_H

#include <iostream>
#include "../../API/PlayerExtensionInterface.h"
#include "glm/gtc/quaternion.hpp"
/**
 * Cowboy attachment of player can be doing one of these:
 * 1) idling
 * 2) walking
 * 3) running
 * 4) shooting
 */
class CowboyShooterExtension : public  PlayerExtensionInterface {
    static PlayerExtensionRegister<CowboyShooterExtension> reg;

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
    glm::vec3 muzzleFlashOffset = glm::vec3(0.010f, 0.170f, 0.517f);
    uint32_t playerAttachedModelID;
    uint32_t playerAttachedPistolID;
    uint32_t playerAttachedRifleID;

    const LimonAPI::Vec4 removeOffset = LimonAPI::Vec4(0, 0, -50);
    const LimonAPI::Vec4 addOffset = LimonAPI::Vec4(0, 0, 50);

    int hitPoints = 100;

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
                limonAPI->addObjectTranslate(playerAttachedRifleID, removeOffset);
                currentGun = Gun::PISTOL;
            }
        }
    }
    void removeDamageIndicator(std::vector<LimonAPI::ParameterRequest> parameters);
    void removeMuzzleFlash(std::vector<LimonAPI::ParameterRequest> parameters);
    void processInput(const InputStates &inputState, long time) override;

    void interact(std::vector<LimonAPI::ParameterRequest> &interactionData) override;

    std::string getName() const override;

};


extern "C" void registerPlayerExtensions(std::map<std::string, PlayerExtensionInterface*(*)(LimonAPI*)>*);






#endif //LIMONENGINE_COWBOYSHOOTEREXTENSION_H
