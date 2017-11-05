//
// Created by Engin Manap on 17.02.2016.
//

#ifndef UBERGAME_CAMERA_H
#define UBERGAME_CAMERA_H


#include <btBulletDynamicsCommon.h>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Options.h"
#include "Utils/GLMConverter.h"

#define STEPPING_TEST_COUNT 5

class Camera {
    const glm::vec3 startPosition = glm::vec3(0, 10, 15);
    bool dirty;
    glm::vec3 position, center, up, right;
    glm::quat view, viewChange;
    glm::mat4 cameraTransformMatrix;
    btRigidBody *player;
    std::vector<btCollisionWorld::ClosestRayResultCallback> rayCallbackArray;
    btTransform worldTransformHolder;
    bool onAir;
    Options *options;
public:
    enum moveDirections {
        NONE, FORWARD, BACKWARD, LEFT, RIGHT, LEFT_FORWARD, RIGHT_FORWARD, LEFT_BACKWARD, RIGHT_BACKWARD, UP
    };

    Camera(Options *options);

    //FIXME there is a typo here
    void updateTransfromFromPhysics(const btDynamicsWorld *world);

    void setCenter(const glm::vec3 &center) {
        glm::vec3 normalizeCenter = glm::normalize(center);
        if (this->center != normalizeCenter) {
            this->center = normalizeCenter;
            this->right = glm::normalize(glm::cross(normalizeCenter, up));
            this->dirty = true;
        }
    }

    void move(moveDirections);

    void rotate(float xChange, float yChange);


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

    btRigidBody *getRigidBody() {
        return player;
    }

};

#endif //UBERGAME_CAMERA_H
