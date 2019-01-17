//
// Created by engin on 27.11.2017.
//

#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include "ActorInterface.h"

std::map<std::string, ActorInterface*(*)(uint32_t, LimonAPI*)>* ActorInterface::typeMap;

glm::vec3 ActorInterface::getFrontVector() const {
    std::vector<LimonAPI::ParameterRequest> parameters = limonAPI->getObjectTransformation(modelID);
    glm::quat rotation(0,0,1,0);
    if(parameters.size() >= 3) {
        rotation = glm::quat(parameters[2].value.vectorValue.x,
                             parameters[2].value.vectorValue.y,
                             parameters[2].value.vectorValue.z,
                             parameters[2].value.vectorValue.w);
    } else {
        std::cerr << "ActorInterface Model transform can't be found for actor " << this->getModelID() << " and model " << modelID << std::endl;
    }

    // Extract the vector part of the quaternion
    glm::vec3 u(rotation.x, rotation.y, rotation.z);
    glm::vec3 forward(0.0f,0.0f,1.0f);
    // Extract the scalar part of the quaternion
    float s = rotation.w;

    // Do the math
    glm::vec3 vprime = 2.0f * glm::dot(u, forward) * u
                       + (s*s - glm::dot(u, u)) * forward
                       + 2.0f * s * glm::cross(u, forward);
    return vprime;
}

glm::vec3 ActorInterface::getPosition() const {
    std::vector<LimonAPI::ParameterRequest> parameters = limonAPI->getObjectTransformation(modelID);
    glm::vec3 position(0,0,0);
    if(parameters.size() >= 1) {
        position = glm::vec3(parameters[0].value.vectorValue.x,
                             parameters[0].value.vectorValue.y,
                             parameters[0].value.vectorValue.z);
    } else {
        std::cerr << "ActorInterface Model transform can't be found for actor " << this->getModelID() << " and model " << modelID << std::endl;
    }
    return position;
}