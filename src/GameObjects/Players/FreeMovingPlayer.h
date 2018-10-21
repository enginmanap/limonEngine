//
// Created by engin on 15.02.2018.
//

#ifndef LIMONENGINE_FREEMOVINGPLAYER_H
#define LIMONENGINE_FREEMOVINGPLAYER_H


#include "../../CameraAttachment.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Player.h"
#include "../../GUI/GUIRenderable.h"

class Options;

class FreeMovingPlayer : public Player, public CameraAttachment {
    bool dirty;
    glm::vec3 position;
    glm::vec3 center;
    glm::vec3 up;
    glm::vec3 right;
    glm::quat view;
public:

    FreeMovingPlayer(Options* options, GUIRenderable* cursor, const glm::vec3 &position,
                     const glm::vec3 &lookDirection);

    bool isDirty() {
        return dirty;
    }

    void getCameraVariables(glm::vec3 &position, glm::vec3 &center, glm::vec3 &up, glm::vec3 &right) {
        position = this->position;
        center = this->center;
        up = this->up;
        right = this->right;
    };

    void move(moveDirections);

    glm::vec3 getPosition() const {
        return position;
    }

    glm::vec3 getLookDirection() const {
        return this->center;
    };

    void getWhereCameraLooks(glm::vec3 &fromPosition, glm::vec3 &lookDirection) const {
        fromPosition = this->getPosition();
        lookDirection = this->center;
    }

    void ownControl(const glm::vec3& position, const glm::vec3 lookDirection) {
        this->position = position;

        this->center = glm::normalize(lookDirection);
        this->right = glm::normalize(glm::cross(center, up));
        this->view.x = this->center.x;
        this->view.y = this->center.y;
        this->view.z = this->center.z;
        this->view.w = 0;

        cursor->setTranslate(glm::vec2(options->getScreenWidth()/2.0f, options->getScreenHeight()/2.0f));
    };

    void registerToPhysicalWorld(btDiscreteDynamicsWorld *world __attribute((unused)), int collisionGroup __attribute((unused)), int collisionMask __attribute((unused)),
                                     const glm::vec3 &worldAABBMin __attribute((unused)), const glm::vec3 &worldAABBMax __attribute((unused))) {}


    void processPhysicsWorld(const btDiscreteDynamicsWorld *world __attribute__((unused))) {};

    void rotate(float xPosition, float yPosition, float xChange, float yChange);

    CameraAttachment* getCameraAttachment() {
        return this;
    }
};


#endif //LIMONENGINE_FREEMOVINGPLAYER_H
