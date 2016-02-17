//
// Created by Engin Manap on 17.02.2016.
//

#ifndef UBERGAME_CAMERA_H
#define UBERGAME_CAMERA_H

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

class Camera {
    bool dirty;
    glm::vec3 position, center, up;
    glm::mat4 cameraTransformMatrix;
public:

    Camera():
            dirty(false),
            position(glm::vec3(0,0,0)),
            center(glm::vec3(0,0,-1)),
            up(glm::vec3(0,1,0)){
            cameraTransformMatrix = glm::lookAt(position, center, up);
    }
    void setPosition(const glm::vec3& position){
        if(this->position != position) {
            this->position = position;
            this->dirty = true;
        }
    }

    void setCenter(const glm::vec3& center){
        if(this->center != center) {
            this->center = center;
            this->dirty = true;
        }
    }

    glm::mat4 getCameraMatrix(){
        if(this->dirty){
            this->cameraTransformMatrix = glm::lookAt(position, center, up);
        }
        return cameraTransformMatrix;
    }
};

#endif //UBERGAME_CAMERA_H
