//
// Created by Engin Manap on 17.02.2016.
//

#ifndef UBERGAME_CAMERA_H
#define UBERGAME_CAMERA_H

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

class Camera {
    float moveSpeed = 0.1;
    float lookAroundSpeed = 1.0f;
    bool dirty;
    glm::vec3 position, center, up, right;
    glm::quat view, viewChange;
    glm::mat4 cameraTransformMatrix;
public:
    enum moveDirections{NONE, FORWARD, BACKWARD, LEFT, RIGHT, LEFT_FORWARD, RIGHT_FORWARD, LEFT_BACKWARD, RIGHT_BACKWARD};
    Camera():
        dirty(false),
        position(glm::vec3(0,0,0)),
        center(glm::vec3(0,0,-1)),
        up(glm::vec3(0,1,0)),
        right(glm::vec3(-1,0,0)),
        view(glm::quat(0,0,0,-1)){
            cameraTransformMatrix = glm::lookAt(position, center, up);
    }
    void setPosition(const glm::vec3& position){
        if(this->position != position) {
            this->position = position;
            this->dirty = true;
        }
    }

    void setCenter(const glm::vec3& center){
        glm::vec3 normalizeCenter = glm::normalize(center);
        if(this->center != normalizeCenter ) {
            this->center = normalizeCenter;
            this->right = glm::cross(normalizeCenter, up);
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
};

#endif //UBERGAME_CAMERA_H
