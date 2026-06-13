//
// Created by engin on 27.11.2017.
//

#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include "ActorInterface.h"

std::map<std::string, ActorInterface*(*)(uint32_t, LimonAPI*)>* ActorInterface::typeMap;

glm::vec3 ActorInterface::getFrontVector() const {
    // Logic lives once in LimonAPI (mirrored in the Python API as get_object_front_vector).
    LimonTypes::Vec4 front = limonAPI->getObjectFrontVector(modelID);
    return glm::vec3(front.x, front.y, front.z);
}

glm::vec3 ActorInterface::getPosition() const {
    // Logic lives once in LimonAPI (mirrored in the Python API as get_object_position).
    LimonTypes::Vec4 position = limonAPI->getObjectPosition(modelID);
    return glm::vec3(position.x, position.y, position.z);
}