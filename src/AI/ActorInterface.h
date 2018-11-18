//
// Created by engin on 27.11.2017.
//

#ifndef LIMONENGINE_ACTOR_H
#define LIMONENGINE_ACTOR_H

#include <glm/detail/type_quat.hpp>
#include "../Options.h"
#include "../GamePlay/LimonAPI.h"



class ActorInterface {
protected:
    uint32_t worldID;
    uint32_t modelID = 0;
    LimonAPI* limonAPI;
public:

    struct ActorInformation{
        bool canSeePlayerDirectly = false;
        bool isPlayerLeft = false, isPlayerRight = false, isPlayerUp = false, isPlayerDown = false, isPlayerFront = false, isPlayerBack = false;
        float cosineBetweenPlayer = 0.0f;
        glm::vec3 playerDirection;
        float cosineBetweenPlayerForSide;
        glm::vec3 toPlayerRoute;
        bool canGoToPlayer = false;
        bool playerDead = false;
    };

    ActorInterface(uint32_t id, LimonAPI *limonAPI) : worldID(id), limonAPI(limonAPI) {}

    virtual void play(long time, ActorInterface::ActorInformation &information, Options* options) = 0;

    virtual bool interaction(std::vector<LimonAPI::ParameterRequest> &interactionInformation) = 0;

    uint32_t getWorldID() {
        return worldID;
    }

    void setModel(uint32_t modelID) {
        this->modelID = modelID;
    }

    uint32_t getModelID() const {
        return modelID;
    }

    glm::vec3 getPosition() const {
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

    glm::vec3 getFrontVector() const {
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

    virtual void IMGuiEditorView() {};

    virtual ~ActorInterface() {};
};


#endif //LIMONENGINE_ACTOR_H
