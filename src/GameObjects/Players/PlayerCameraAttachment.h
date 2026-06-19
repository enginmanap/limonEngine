//
// Created by engin on 2026.
//

#ifndef LIMONENGINE_PLAYERCAMERAATTACHMENT_H
#define LIMONENGINE_PLAYERCAMERAATTACHMENT_H

#include <glm/vec3.hpp>

#include "limonAPI/CameraAttachment.h"

/**
 * The default camera attachment every Player owns.
 *
 * The player computes its own eye pose (from physics / free-fly / cursor state) and PUSHES it here via
 * setPose() each frame; this attachment merely stores and returns it. That keeps a Player from BEING a
 * camera — it no longer subclasses CameraAttachment — and collapses the four per-player
 * getCameraVariables/isDirty/clearDirty copies into this single class.
 *
 * Players drive a standard perspective projection by default; getProjection() returns the stored
 * ProjectionParameters. A registered CameraRig overrides the player's camera via Player::setCameraOverride,
 * in which case this attachment is simply not the one bound to the player camera.
 */
class PlayerCameraAttachment : public CameraAttachment {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 center   = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up       = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right    = glm::vec3(-1.0f, 0.0f, 0.0f);
    ProjectionParameters projectionParameters; // default perspective
    bool dirty = true;

public:
    void setPose(const glm::vec3& position, const glm::vec3& center, const glm::vec3& up, const glm::vec3& right) {
        this->position = position;
        this->center   = center;
        this->up       = up;
        this->right    = right;
        this->dirty    = true;
    }

    void setProjection(const ProjectionParameters& projectionParameters) {
        this->projectionParameters = projectionParameters;
        this->dirty = true;
    }

    bool isDirty() const override {
        return dirty;
    }

    void clearDirty() override {
        dirty = false;
    }

    void getCameraVariables(glm::vec3& position, glm::vec3& center, glm::vec3& up, glm::vec3& right) override {
        position = this->position;
        center   = this->center;
        up       = this->up;
        right    = this->right;
    }

    ProjectionParameters getProjection() const override {
        return projectionParameters;
    }
};

#endif //LIMONENGINE_PLAYERCAMERAATTACHMENT_H
