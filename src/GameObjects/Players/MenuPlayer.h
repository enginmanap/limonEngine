//
// Created by engin on 28.07.2018.
//

#ifndef LIMONENGINE_MENUPLAYER_H
#define LIMONENGINE_MENUPLAYER_H


#include "../../CameraAttachment.h"
#include "Player.h"
#include <glm/gtx/quaternion.hpp>

class Options;
class GUIRenderable;

class MenuPlayer: public Player, public CameraAttachment {
    bool dirty;
    glm::vec3 position;
    glm::vec3 center;
    glm::vec3 up;
    glm::vec3 right;
    glm::quat view;
public:

    MenuPlayer(Options* options, GUIRenderable* cursor, const glm::vec3 &position,
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
    };

    void registerToPhysicalWorld(btDiscreteDynamicsWorld *world [[gnu::unused]], int collisionGroup [[gnu::unused]], int collisionMaskForSelf [[gnu::unused]],
                                     int collisionMaskForGround [[gnu::unused]], const glm::vec3 &worldAABBMin [[gnu::unused]], const glm::vec3 &worldAABBMax [[gnu::unused]]) {}

    void processPhysicsWorld(const btDiscreteDynamicsWorld *world __attribute__((unused))) {};

    void rotate(float xPosition, float yPosition, float xChange, float yChange);

    CameraAttachment* getCameraAttachment() {
        return this;
    }

    void processInput(const InputStates &inputHandler, long time) {
        Player::processInput(inputHandler, time);

        if(playerExtension != nullptr) {
            playerExtension->processInput(inputHandler, time);
        }
    }

    void interact(LimonAPI *limonAPI __attribute__((unused)), std::vector<LimonAPI::ParameterRequest> &interactionData) {
        if(playerExtension != nullptr) {
            playerExtension->interact(interactionData);
        }
    }
};


#endif //LIMONENGINE_MENUPLAYER_H
