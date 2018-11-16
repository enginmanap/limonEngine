//
// Created by engin on 16.11.2018.
//

#ifndef LIMONENGINE_SHOOTERPLAYEREXTENSION_H
#define LIMONENGINE_SHOOTERPLAYEREXTENSION_H


#include "PlayerExtensionInterface.h"

class ShooterPlayerExtension : public  PlayerExtensionInterface {
    static PlayerExtensionRegister<ShooterPlayerExtension> reg;

    uint32_t playerAttachedModelID;
    uint32_t addedElement = 0;

    uint32_t removeCounter = 0;
public:

    ShooterPlayerExtension(LimonAPI* limonAPI) : PlayerExtensionInterface(limonAPI) {
        playerAttachedModelID = limonAPI->getPlayerAttachedModel();
    }
    void removeDamageIndicator(std::vector<LimonAPI::ParameterRequest> parameters);
    void processInput(InputHandler &inputHandler) override;

    void interact(std::vector<LimonAPI::ParameterRequest> &interactionData) override;

    std::string getName() const override;

};


extern "C" void registerPlayerExtensions(std::map<std::string, PlayerExtensionInterface*(*)(LimonAPI*)>*);



#endif //LIMONENGINE_SHOOTERPLAYEREXTENSION_H
