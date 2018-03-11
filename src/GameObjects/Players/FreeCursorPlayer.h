//
// Created by engin on 27.02.2018.
//

#ifndef LIMONENGINE_FREECURSORPLAYER_H
#define LIMONENGINE_FREECURSORPLAYER_H

#include "Player.h"
#include "../../CameraAttachment.h"
#include "../../Utils/GLMUtils.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


class Options;
class GUIRenderable;

class FreeCursorPlayer : public Player, public CameraAttachment {
    Options* options;
    bool dirty;
    glm::vec3 position;
    glm::vec3 center;
    glm::vec3 up;
    glm::vec3 right;
    glm::quat view;
    GUIRenderable* cursor;
    glm::vec3 toRay;
public:

    FreeCursorPlayer(Options* options, GUIRenderable* cursor);

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

    void getWhereCameraLooks(glm::vec3 &fromPosition, glm::vec3 &toPosition) const;

    glm::vec3 getLookDirection() const {
        return this->center;
    };

    void setPositionAndRotation(const glm::vec3& position, const glm::vec3 lookDirection) {
        this->position = position;

        this->center = glm::normalize(lookDirection);
        this->right = glm::normalize(glm::cross(center, up));
        this->view = this->center;

    };

    void registerToPhysicalWorld(btDiscreteDynamicsWorld* world __attribute__((unused)), const glm::vec3& worldAABBMin __attribute__((unused)), const glm::vec3& worldAABBMax __attribute__((unused))) {}
    void processPhysicsWorld(const btDiscreteDynamicsWorld *world __attribute__((unused))) {};

    void rotate(float xPosition, float yPosition, float xChange, float yChange);
};


#endif //LIMONENGINE_FREECURSORPLAYER_H
