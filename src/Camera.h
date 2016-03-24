//
// Created by Engin Manap on 17.02.2016.
//

#ifndef UBERGAME_CAMERA_H
#define UBERGAME_CAMERA_H


#include <btBulletDynamicsCommon.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

#include "Utils/BulletGLMConverter.h"

class Camera {
    const glm::vec3 startPosition = glm::vec3(0,10,15);
    glm::vec3 moveSpeed = glm::vec3(3,0,3);
    float jumpFactor = 10.0f;
    float lookAroundSpeed = 1.0f;
    bool dirty;
    glm::vec3 position, center, up, right;
    glm::quat view, viewChange;
    glm::mat4 cameraTransformMatrix;
    btRigidBody* player;
    btCollisionWorld::ClosestRayResultCallback rayCallback;
    btTransform worldTransformHolder;
    bool onAir;
public:
    enum moveDirections{NONE, FORWARD, BACKWARD, LEFT, RIGHT, LEFT_FORWARD, RIGHT_FORWARD, LEFT_BACKWARD, RIGHT_BACKWARD, UP};
    Camera();
    void updateTransfromFromPhysics(const btDynamicsWorld* world);

    void setCenter(const glm::vec3& center){
        glm::vec3 normalizeCenter = glm::normalize(center);
        if(this->center != normalizeCenter ) {
            this->center = normalizeCenter;
            this->right = glm::normalize(glm::cross(normalizeCenter, up));
            this->dirty = true;
        }
    }

    void move(moveDirections);
    void rotate(float xChange, float yChange);


    glm::mat4 getCameraMatrix(){
        if(this->dirty){
            this->cameraTransformMatrix = glm::lookAt(position, center+position, up);
            this->dirty = false;
        }
        return cameraTransformMatrix;
    }

    btRigidBody *getRigidBody() {
        return player;
    }
};

#endif //UBERGAME_CAMERA_H
