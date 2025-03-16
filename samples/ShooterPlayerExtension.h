//
// Created by engin on 16.11.2018.
//

#ifndef LIMONENGINE_SHOOTERPLAYEREXTENSION_H
#define LIMONENGINE_SHOOTERPLAYEREXTENSION_H


#include "limonAPI/PlayerExtensionInterface.h"

class ShooterPlayerExtension : public  PlayerExtensionInterface {
    static const glm::quat direction;
    glm::vec3 muzzleFlashOffset = glm::vec3(-0.18f,2.85f,0.5750f);
    uint32_t playerAttachedModelID;
    uint32_t addedElement = 0;

    uint32_t removeCounter = 0;
    float bulletForce = 1000;
    int hitPoints = 100;
public:

    ShooterPlayerExtension(LimonAPI* limonAPI) : PlayerExtensionInterface(limonAPI) {
        playerAttachedModelID = limonAPI->getPlayerAttachedModel();
    }
    void removeDamageIndicator(std::vector<LimonTypes::GenericParameter> parameters);
    void processInput(const InputStates &inputState, const PlayerExtensionInterface::PlayerInformation &playerInformation,
                          long time) override;

    void interact(std::vector<LimonTypes::GenericParameter> &interactionData) override;

    std::string getName() const override;

};


extern "C" void registerPlayerExtensions(std::map<std::string, PlayerExtensionInterface*(*)(LimonAPI*)>*);



#endif //LIMONENGINE_SHOOTERPLAYEREXTENSION_H
