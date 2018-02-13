//
// Created by Engin Manap on 17.02.2016.
//

#include "Camera.h"


Camera::Camera(Options *options) :
        dirty(true),
        position(startPosition),
        center(glm::vec3(0, 0, -1)),
        up(glm::vec3(0, 1, 0)),
        right(glm::vec3(-1, 0, 0)),
        view(glm::quat(0, 0, 0, -1)),
        options(options) {

    cameraTransformMatrix = glm::lookAt(position, center, up);
}