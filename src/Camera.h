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


class Camera {
    const glm::vec3 startPosition = glm::vec3(0, 10, 15);
    bool dirty;
    glm::vec3 position, center, up, right;
    glm::quat view;
    glm::mat4 cameraTransformMatrix;

    Options *options;
public:

    explicit Camera(Options *options);


    void setCenter(const glm::vec3 &center) {
        glm::vec3 normalizeCenter = glm::normalize(center);
        if (this->center != normalizeCenter) {
            this->center = normalizeCenter;
            this->right = glm::normalize(glm::cross(normalizeCenter, up));
            this->dirty = true;
        }
    }

    void setPosition(const glm::vec3& position) {
        this->position = position;
    }

    void rotate(float xChange, float yChange) {
        std::cout << "camera rotation " << std::endl;
        glm::quat viewChange;
        viewChange = glm::quat(cos(yChange * options->getLookAroundSpeed() / 2),
                               right.x * sin(yChange * options->getLookAroundSpeed() / 2),
                               right.y * sin(yChange * options->getLookAroundSpeed() / 2),
                               right.z * sin(yChange * options->getLookAroundSpeed() / 2));

        view = viewChange * view * glm::conjugate(viewChange);
        view = glm::normalize(view);

        viewChange = glm::quat(cos(xChange * options->getLookAroundSpeed() / 2),
                               up.x * sin(xChange * options->getLookAroundSpeed() / 2),
                               up.y * sin(xChange * options->getLookAroundSpeed() / 2),
                               up.z * sin(xChange * options->getLookAroundSpeed() / 2));
        view = viewChange * view * glm::conjugate(viewChange);
        view = glm::normalize(view);

        center.x = view.x;
        if (view.y > 1.0f) {
            center.y = 0.9999f;
        } else if (view.y < -1.0f) {
            center.y = -0.9999f;
        } else {
            center.y = view.y;
        }
        center.z = view.z;
        center = glm::normalize(center);
        right = glm::normalize(glm::cross(center, up));
        dirty = true;
    }

    glm::mat4 getCameraMatrix() {
        if (this->dirty) {
            this->cameraTransformMatrix = glm::lookAt(position, center + position, up);
            this->dirty = false;
        }
        return cameraTransformMatrix;
    }

    bool isDirty() const {
        return dirty;
    }

    glm::vec3 const getPosition() {
        return position;
    }

};


#endif //LIMONENGINE_CAMERA_H
