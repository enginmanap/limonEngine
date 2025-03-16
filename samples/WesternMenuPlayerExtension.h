//
// Created by engin on 9.01.2019.
//

#ifndef LIMONENGINE_WESTERNMENUPLAYEREXTENSION_H
#define LIMONENGINE_WESTERNMENUPLAYEREXTENSION_H


#include <random>
#include "limonAPI/PlayerExtensionInterface.h"

class WesternMenuPlayerExtension : public PlayerExtensionInterface {
    std::uniform_real_distribution<float> randomFloatsDirection; // generates random floats between 0.0 and 1.0
    std::uniform_real_distribution<float> randomFloatsSpeed; // generates random floats between 0.3 and 1.0
    std::default_random_engine generator;

    glm::vec3 direction;
    glm::vec3 addedPositionTillNow = glm::vec3(0,0,0);
    long startTime = 0;
    LimonTypes::Vec4 color = LimonTypes::Vec4(0.5f, 0.5f, 0.5f, 0);
    float speed;
public:

    WesternMenuPlayerExtension(LimonAPI* limonAPI) : PlayerExtensionInterface(limonAPI) {
        randomFloatsDirection = std::uniform_real_distribution<float> (0.0f, 1.0); // generates random floats between 0.0 and 1.0
        randomFloatsSpeed = std::uniform_real_distribution<float> (0.5f, 3.0); // generates random floats between 0.0 and 1.0

    }
    void processInput(const InputStates &inputState, const PlayerExtensionInterface::PlayerInformation &playerInformation,
                          long time) override;

    void interact(std::vector<LimonTypes::GenericParameter> &interactionData) override;

    std::string getName() const override;
};


#endif //LIMONENGINE_WESTERNMENUPLAYEREXTENSION_H
