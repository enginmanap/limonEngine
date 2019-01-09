//
// Created by engin on 9.01.2019.
//

#include <glm/ext.hpp>

#include "WesternMenuPlayerExtension.h"
#include "../../Utils/LimonConverter.h"

PlayerExtensionRegister<WesternMenuPlayerExtension> WesternMenuPlayerExtension::reg("WesternMenuExtension");

void WesternMenuPlayerExtension::processInput(InputHandler &inputHandler [[gnu::unused]], long time) {
    static constexpr float PI = 3.14159265358979f;

    //This method will update light info for the camp fire

    /**
     * select a random direction vector, with length 0.2
     * select a random time between 0.5 to 3 second
     * move that direction with sin()
    **/
    if(startTime == 0) {
        float directionX = randomFloatsDirection(generator);
        float directionY = randomFloatsDirection(generator);
        float directionZ = randomFloatsDirection(generator);
        direction = glm::vec3(directionX, directionY, directionZ);
        direction = glm::normalize(direction);
        direction = direction / 5.0f;
        addedPositionTillNow = glm::vec3(0, 0, 0);
        startTime = time;
        speed = randomFloatsSpeed(generator);
    }

    float positionFactor = std::sin((time-startTime) / (speed*1000));


    glm::vec3 currentDirection = direction* positionFactor;//current direction is what we want. remove added pos
    glm::vec3 vectorToAdd = currentDirection - addedPositionTillNow;
    limonAPI->addLightTranslate(3, LimonConverter::GLMToLimon(vectorToAdd));

    addedPositionTillNow = currentDirection;

    if(((time - startTime) / (speed*1000)) > 2*PI) {
        startTime = 0;//so will randomize again
    }
}

void WesternMenuPlayerExtension::interact(std::vector<LimonAPI::ParameterRequest> &interactionData [[gnu::unused]]) {

}

std::string WesternMenuPlayerExtension::getName() const {
    return "WesternMenuExtension";
}
