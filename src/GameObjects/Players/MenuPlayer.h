//
// Created by engin on 28.07.2018.
//

#ifndef LIMONENGINE_MENUPLAYER_H
#define LIMONENGINE_MENUPLAYER_H


#include "LimonAPI/CameraAttachment.h"
#include "Player.h"
#include <glm/gtx/quaternion.hpp>
#include <Utils/GLMConverter.h>

class Options;
class GUIRenderable;

class MenuPlayer: public Player {
    bool dirty;
    glm::vec3 position;
    glm::vec3 center;
    glm::vec3 up;
    glm::vec3 right;
    glm::quat view;
public:

    MenuPlayer(OptionsUtil::Options* options, GUIRenderable* cursor, const glm::vec3 &position,
               const glm::vec3 &lookDirection);

    bool isDirty() const override {
        return dirty;
    }

    void clearDirty() override {
        this->dirty = false;
    }

    void getCameraVariables(glm::vec3 &position, glm::vec3 &center, glm::vec3 &up, glm::vec3 &right) override {
        position = this->position;
        center = this->center;
        up = this->up;
        right = this->right;
    };

    void move(moveDirections) override;

    glm::vec3 getPosition() const override {
        return position;
    }

    void getWhereCameraLooks(glm::vec3 &fromPosition, glm::vec3 &toPosition) const override;

    glm::vec3 getLookDirection() const override {
        return this->center;
    };

    /**
     * This method allows player class to setup what it needs to act properly.
     *
     * @param position
     * @param lookDirection
     */
    void ownControl(const glm::vec3 &position, const glm::vec3 &lookDirection) override {
        this->position = position;

        this->center = glm::normalize(lookDirection);
        this->right = glm::normalize(glm::cross(center, up));
        this->view.x = this->center.x;
        this->view.y = this->center.y;
        this->view.z = this->center.z;
        this->view.w = 0;
    };

    void rotate(float xPosition, float yPosition, float xChange, float yChange) override;

    CameraAttachment* getCameraAttachment() override {
        return this;
    }

    void processInput(const InputStates &inputHandler, long time) override {
        Player::processInput(inputHandler, time);

        if(playerExtension != nullptr) {
            PlayerExtensionInterface::PlayerInformation playerInformation;
            playerInformation.position = GLMConverter::GLMToLimon(this->getPosition());
            playerInformation.lookDirection = GLMConverter::GLMToLimon(this->getLookDirection());
            playerExtension->processInput(inputHandler, playerInformation, time);
        }
    }

    void interact(LimonAPI *limonAPI __attribute__((unused)), std::vector<LimonTypes::GenericParameter> &interactionData) override {
        if(playerExtension != nullptr) {
            playerExtension->interact(interactionData);
        }
    }
};


#endif //LIMONENGINE_MENUPLAYER_H
