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
#include <SDL2/SDL.h>


class Options;
class GUIRenderable;

class FreeCursorPlayer : public Player, public CameraAttachment {
    bool dirty;
    glm::vec3 position;
    glm::vec3 center;
    glm::vec3 up;
    glm::vec3 right;
    glm::quat view;
public:

    FreeCursorPlayer(Options *options, GUIRenderable *cursor, const glm::vec3 &position,
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

    void getWhereCameraLooks(glm::vec3 &fromPosition, glm::vec3 &toPosition) const;

    glm::vec3 getLookDirection() const {
        return this->center;
    };

    /**
     * This method allows player class to setup what it needs to act properly.
     *
     * @param position
     * @param lookDirection
     */
    void ownControl(const glm::vec3 &position, const glm::vec3 lookDirection) {
        this->position = position;

        this->center = glm::normalize(lookDirection);
        this->right = glm::normalize(glm::cross(center, up));
        this->view.x = this->center.x;
        this->view.y = this->center.y;
        this->view.z = this->center.z;
        this->view.w = 0;
        //FIXME Do we need release control?
    };

    void registerToPhysicalWorld(btDiscreteDynamicsWorld *world [[gnu::unused]], int collisionGroup [[gnu::unused]], int collisionMaskForSelf [[gnu::unused]],
                                     int collisionMaskForGround [[gnu::unused]], const glm::vec3 &worldAABBMin [[gnu::unused]], const glm::vec3 &worldAABBMax [[gnu::unused]]) {}
    void processPhysicsWorld(const btDiscreteDynamicsWorld *world [[gnu::unused]]) {};

    void rotate(float xPosition, float yPosition, float xChange, float yChange);

    CameraAttachment* getCameraAttachment() {
        return this;
    }
};


#endif //LIMONENGINE_FREECURSORPLAYER_H
