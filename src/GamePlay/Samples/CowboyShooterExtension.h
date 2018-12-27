//
// Created by engin on 26.12.2018.
//

#ifndef LIMONENGINE_COWBOYSHOOTEREXTENSION_H
#define LIMONENGINE_COWBOYSHOOTEREXTENSION_H

#include "../PlayerExtensionInterface.h"
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

    State currentState = State::IDLE;
    State transitionState = State::IDLE;

    std::string currentAnimationName;
    bool currentAnimationFinished;

    const glm::quat direction = glm::quat(0.0f, 0.0f, 1.0f, 0.0f);//this is used to reverse hit normal
    glm::vec3 muzzleFlashOffset = glm::vec3(0.010f,0.170f,0.517f);
    uint32_t playerAttachedModelID;
    uint32_t playerAttachedGunID;

    int hitPoints = 100;

    void shootingTransition();
    void walkingTransition();
    void runningTransition();
    void idleTransition();
public:

    CowboyShooterExtension(LimonAPI* limonAPI) : PlayerExtensionInterface(limonAPI) {
        playerAttachedModelID = limonAPI->getPlayerAttachedModel();
        std::vector<uint32_t > children = limonAPI->getModelChildren(playerAttachedModelID);
        if(children.size() == 0) {
            std::cerr << "player attachment has no child, it should have a gun!" << std::endl;
            playerAttachedGunID = 0;
        } else {
            playerAttachedGunID = children[0];
        }
    }
    void removeDamageIndicator(std::vector<LimonAPI::ParameterRequest> parameters);
    void removeMuzzleFlash(std::vector<LimonAPI::ParameterRequest> parameters);
    void processInput(InputHandler &inputHandler) override;

    void interact(std::vector<LimonAPI::ParameterRequest> &interactionData) override;

    std::string getName() const override;

};


extern "C" void registerPlayerExtensions(std::map<std::string, PlayerExtensionInterface*(*)(LimonAPI*)>*);






#endif //LIMONENGINE_COWBOYSHOOTEREXTENSION_H
