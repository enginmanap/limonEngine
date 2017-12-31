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

#define STEPPING_TEST_COUNT 5

class Camera {
    std::string objectType = "camera";//FIXME this is just temporary ray test result detection code, we should return game objects instead of string
    const glm::vec3 startPosition = glm::vec3(0, 10, 15);
    float slowDownFactor = 2.5f;

    bool dirty;
    glm::vec3 position, center, up, right;
    glm::quat view, viewChange;
    glm::mat4 cameraTransformMatrix;
    btRigidBody *player;
    btGeneric6DofSpring2Constraint *spring;
    float springStandPoint;

    std::vector<btCollisionWorld::ClosestRayResultCallback> rayCallbackArray;
    btTransform worldTransformHolder;
    bool onAir;
    Options *options;
public:
    enum moveDirections {
        NONE, FORWARD, BACKWARD, LEFT, RIGHT, LEFT_FORWARD, RIGHT_FORWARD, LEFT_BACKWARD, RIGHT_BACKWARD, UP
    };

    Camera(Options *options);
    ~Camera() {
        delete player;
        if(spring != NULL) {
            delete spring;
        }
    }

    void updateTransformFromPhysics(const btDynamicsWorld *world);

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

    /**
     * This method requires the world, because it raytests for closest object below the camera.
     * This is required because single sided spring constrain automatically attaches to world itself,
     * and we need to calculate an equilibrium point.
     *
     * @param world
     * @return
     */
    btGeneric6DofSpring2Constraint *getSpring(float minY);
};

#endif //LIMONENGINE_CAMERA_H
