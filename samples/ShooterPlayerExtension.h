//
// Created by engin on 16.11.2018.
//

#ifndef LIMONENGINE_SHOOTERPLAYEREXTENSION_H
#define LIMONENGINE_SHOOTERPLAYEREXTENSION_H


#include "limonAPI/PlayerExtensionInterface.h"

class ShooterPlayerExtension : public  PlayerExtensionInterface {
    static const glm::quat direction;
    glm::vec3 muzzleFlashOffset = glm::vec3(-0.18f,2.85f,0.5750f);
    uint32_t playerAttachedBodyID = 0;
    uint32_t addedElement = 0;

    uint32_t removeCounter = 0;
    float bulletForce = 1000;
    int hitPoints = 100;
public:
    static std::vector<std::string> getPackFileAccessList() {
        return {"./Data/Sounds/EasyFPS/shot.wav",
                "./Data/Models/Muzzle/Muzzle.obj",
                "./Data/Models/BulletHole/BulletHole.obj",
                "./Data/Textures/BloodParticle.png",
                "./Data/Textures/BaseParticle.png",
                "./Data/Textures/damageIndicator.png"};
    }


    ShooterPlayerExtension(LimonAPI* limonAPI) : PlayerExtensionInterface(limonAPI) {
        LimonTypes::GenericParameter bodyParameter;
        bodyParameter.requestType = LimonTypes::GenericParameter::RequestParameterTypes::MODEL;
        bodyParameter.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
        bodyParameter.description = "Body";
        bodyParameter.isSet = false;
        this->parameters.push_back(bodyParameter);
    }

    void setParameters(std::vector<LimonTypes::GenericParameter> parameters) override;
    void removeDamageIndicator(std::vector<LimonTypes::GenericParameter> parameters);
    void processInput(const InputStates &inputState, const PlayerExtensionInterface::PlayerInformation &playerInformation,
                          uint32_t time) override;

    void interact(std::vector<LimonTypes::GenericParameter> &interactionData) override;

    std::string getName() const override;

};


extern "C" void registerPlayerExtensionTypes(std::map<std::string, PlayerExtensionInterface::Registration>*);



#endif //LIMONENGINE_SHOOTERPLAYEREXTENSION_H
