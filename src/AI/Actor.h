//
// Created by engin on 27.11.2017.
//

#ifndef LIMONENGINE_ACTOR_H
#define LIMONENGINE_ACTOR_H


#include "../Model.h"
#include "../Options.h"

struct ActorInformation{
    bool canSeePlayerDirectly = false;
    bool isPlayerLeft = false, isPlayerRight = false, isPlayerUp = false, isPlayerDown = false, isPlayerFront = false, isPlayerBack = false;
    float cosineBetweenPlayer = 0.0f;
    glm::vec3 playerDirection;
    float cosineBetweenPlayerForSide;
    glm::vec3 toPlayerRoute;
    bool canGoToPlayer = false;
};

class Actor {
protected:
    Model* model = nullptr;
public:
    virtual void play(long time, ActorInformation &information, Options* options) = 0;

    void setModel(Model *model) {
        this->model = model;//FIXME it looks like removing this object will be an issue because it is not clear who clears it
    }

    glm::vec3 getPosition(){
        return GLMConverter::BltToGLM(this->model->getRigidBody()->getCenterOfMassPosition());
    }

    glm::vec3 getFrontVector(){
        btTransform transform = this->model->getRigidBody()->getWorldTransform();
        btQuaternion rotation = transform.getRotation();
        // Extract the vector part of the quaternion
        glm::vec3 u(rotation.getX(), rotation.getY(), rotation.getZ());
        glm::vec3 forward(0.0f,0.0f,1.0f);
        // Extract the scalar part of the quaternion
        float s = rotation.getW();

        // Do the math
        glm::vec3 vprime = 2.0f * glm::dot(u, forward) * u
                 + (s*s - glm::dot(u, u)) * forward
                 + 2.0f * s * glm::cross(u, forward);
        return vprime;
    }

    virtual ~Actor() {};
};


#endif //LIMONENGINE_ACTOR_H
