//
// Created by engin on 13.05.2018.
//

#include "LimonAPI.h"
#include "../World.h"

//this is because the variable is static
World* LimonAPI::world;

void LimonAPI::setWorld(World *inputWorld) {
        world = inputWorld;
}

void LimonAPI::animateModel(uint32_t modelID, uint32_t animationID, bool looped) {
    world->addAnimationToObject(modelID, animationID, looped);
}

bool LimonAPI::generateEditorElementsForParameters(std::vector<LimonAPI::ParameterRequest> &runParameters) {
    return world->generateEditorElementsForParameters(runParameters);
}

void LimonAPI::addGuiText(const std::string &fontFilePath, uint32_t fontSize, const std::string &text,
                          const glm::vec3 &color, const glm::vec2 &position, float rotation) {
    world->addGuiText(fontFilePath, fontSize, text,color, position,rotation);

}
