//
// Created by engin on 15.02.2018.
//

#ifndef LIMONENGINE_FREEMOVINGPLAYER_H
#define LIMONENGINE_FREEMOVINGPLAYER_H


#include "LimonAPI/CameraAttachment.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Player.h"
#include "../../GUI/GUIRenderable.h"

class Options;

class FreeMovingPlayer : public Player {
    bool dirty;
    glm::vec3 position;
    glm::vec3 center;
    glm::vec3 up;
    glm::vec3 right;
    glm::quat view;
public:

    FreeMovingPlayer(OptionsUtil::Options* options, GUIRenderable* cursor, const glm::vec3 &position,
                     const glm::vec3 &lookDirection);

    bool isDirty() const override {
        return dirty;
    }

    void clearDirty() override {
        this->dirty = false;
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

    void ownControl(const glm::vec3& position, const glm::vec3 &lookDirection) override {
        this->position = position;

        this->center = glm::normalize(lookDirection);
        this->right = glm::normalize(glm::cross(center, up));
        this->view.x = this->center.x;
        this->view.y = this->center.y;
        this->view.z = this->center.z;
        this->view.w = 0;

        cursor->setTranslate(glm::vec2(options->getScreenWidth()/2.0f, options->getScreenHeight()/2.0f));
    };

    void rotate(float xPosition, float yPosition, float xChange, float yChange);
};


#endif //LIMONENGINE_FREEMOVINGPLAYER_H
