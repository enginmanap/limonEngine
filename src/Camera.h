//
// Created by Engin Manap on 17.02.2016.
//

#ifndef LIMONENGINE_CAMERA_H
#define LIMONENGINE_CAMERA_H


#include <btBulletDynamicsCommon.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Options.h"
#include "Utils/GLMConverter.h"
#include "Utils/GLMUtils.h"
#include "CameraAttachment.h"


class Camera {
    const glm::vec3 startPosition = glm::vec3(0, 10, 15);
    glm::vec3 position, center, up, right;
    glm::quat view;
    glm::mat4 cameraTransformMatrix;
    CameraAttachment* cameraAttachment;

    Options *options;
public:

    Camera(Options *options, CameraAttachment* cameraAttachment);

    void setCameraAttachment(CameraAttachment *cameraAttachment) {
        Camera::cameraAttachment = cameraAttachment;
    }

    glm::mat4 getCameraMatrix() {
        if (cameraAttachment->isDirty()) {
            cameraAttachment->getCameraVariables(position, center, up, right);
            this->cameraTransformMatrix = glm::lookAt(position, position + center, up);
        }
        return cameraTransformMatrix;
    }

    bool isDirty() const {
        return cameraAttachment->isDirty();
    }

    glm::vec3 const getPosition() const {
        return position;
    }

    glm::vec3 const getCenter() const {
        return center;
    }

    const glm::vec3 &getUp() const {
        return up;
    }

};


#endif //LIMONENGINE_CAMERA_H
