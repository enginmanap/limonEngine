//
// Created by engin on 27.02.2018.
//

#ifndef LIMONENGINE_FREECURSORPLAYER_H
#define LIMONENGINE_FREECURSORPLAYER_H

#include "Player.h"
#include "limonAPI/CameraAttachment.h"
#include "../../Utils/GLMUtils.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <SDL2/SDL.h>


class Options;
class GUIRenderable;

class FreeCursorPlayer : public Player {
    bool dirty;
    glm::vec3 position;
    glm::vec3 center;
    glm::vec3 up;
    glm::vec3 right;
    glm::quat view;
public:

    FreeCursorPlayer(OptionsUtil::Options *options, GUIRenderable *cursor, const glm::vec3 &position,
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

    glm::vec3 getPosition() const override {
        return position;
    }

    void getWhereCameraLooks(glm::vec3 &fromPosition, glm::vec3 &toPosition) const;

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
        //FIXME Do we need release control?
    };

    void rotate(float xPosition, float yPosition, float xChange, float yChange) override;
};


#endif //LIMONENGINE_FREECURSORPLAYER_H
