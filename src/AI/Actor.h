//
// Created by engin on 27.11.2017.
//

#ifndef LIMONENGINE_ACTOR_H
#define LIMONENGINE_ACTOR_H


#include "../GameObjects/Model.h"
#include "../Options.h"

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

class Actor {
protected:
    uint32_t worldID;
    Model* model = nullptr;
    LimonAPI* limonAPI;
public:

    Actor(uint32_t id, LimonAPI *limonAPI) : worldID(id), limonAPI(limonAPI) {}

    virtual void play(long time, ActorInformation &information, Options* options) = 0;

    virtual bool interaction(std::vector<LimonAPI::ParameterRequest> &interactionInformation) = 0;

    uint32_t getWorldID() {
        return worldID;
    }

    void setModel(Model *model) {
        this->model = model;
        model->attachAI(this);
    }

    const Model * getModel() const {
        return model;
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

    virtual void IMGuiEditorView() {};

    virtual ~Actor() {};
};


#endif //LIMONENGINE_ACTOR_H
