//
// Created by Engin Manap on 17.02.2016.
//

#include <c++/5.3.0/iostream>
#include "Camera.h"

void Camera::move(moveDirections direction) {
    if(direction == NONE) {
        return;
    }
    dirty=true;
    switch (direction) {
        case LEFT_BACKWARD:
            position -= moveSpeed * center;
            position -= moveSpeed * right;
            break;
        case LEFT_FORWARD:
            position += moveSpeed * center;
            position -= moveSpeed * right;
            break;
        case LEFT:
            position -= moveSpeed * right;
            break;
        case RIGHT_BACKWARD:
            position -= moveSpeed * center;
            position += moveSpeed * right;
            break;
        case RIGHT_FORWARD:
            position += moveSpeed * center;
            position += moveSpeed * right;
            break;
        case RIGHT:
            position += moveSpeed * right;
            break;
        case BACKWARD:
            position -= moveSpeed * center;
            break;
        case FORWARD:
            position += moveSpeed * center;
            break;
    }
}

void Camera::rotate(float xChange, float yChange){
    viewChange = glm::quat(cos(xChange*lookAroundSpeed/2),0,1*sin(xChange*lookAroundSpeed/2),0);
    view = viewChange * view;
    view = glm::normalize(view);
    center.x = view.x;
    center.y = view.y;
    center.z = view.z;
    right = glm::cross(center, up);
    this->dirty=true;
}