//
// Created by engin on 15.02.2018.
//

#ifndef LIMONENGINE_FREEMOVINGPLAYER_H
#define LIMONENGINE_FREEMOVINGPLAYER_H


#include "../CameraAttachment.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Player.h"

class Options;

class FreeMovingPlayer : public Player, public CameraAttachment {
    Options* options;
    bool dirty;
    glm::vec3 position;
    glm::vec3 center;
    glm::vec3 up;
    glm::vec3 right;
    glm::quat view;
public:

    FreeMovingPlayer(Options* options);

    bool isDirty() {
        return dirty;
    }

    void getCameraVariables(glm::vec3& position, glm::vec3 &center, glm::vec3& up, glm::vec3 right) {
        position = this->position;
        center = this->center;
        up = this->up;
        right = this->right;
    };

    void move(moveDirections);

    glm::vec3 getPosition() const {
        return position;
    }

    void getWhereCameraLooks(glm::vec3 &fromPosition, glm::vec3 &lookDirection) const {
        fromPosition = this->getPosition();
        lookDirection = this->center;
    }

    void registerToPhysicalWorld(btDiscreteDynamicsWorld* world __attribute__((unused)), const glm::vec3& worldAABBMin __attribute__((unused)), const glm::vec3& worldAABBMax __attribute__((unused))) {}
    void processPhysicsWorld(const btDiscreteDynamicsWorld *world __attribute__((unused))) {};

    void rotate(float xChange, float yChange);

};


#endif //LIMONENGINE_FREEMOVINGPLAYER_H
