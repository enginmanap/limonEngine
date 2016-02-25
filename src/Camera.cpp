//
// Created by Engin Manap on 17.02.2016.
//

#include "Camera.h"

void Camera::move(moveDirections direction) {
    if(direction == NONE) {
        return;
    }
    dirty=true;
    switch (direction) {
        case LEFT_BACKWARD:
            position.x -= moveSpeed;
            position.z -= moveSpeed;
            break;
        case LEFT_FORWARD:
            position.x -= moveSpeed;
            position.z += moveSpeed;
            break;
        case LEFT:
            position.x -= moveSpeed;
            break;
        case RIGHT_BACKWARD:
            position.x += moveSpeed;
            position.z -= moveSpeed;
            break;
        case RIGHT_FORWARD:
            position.x += moveSpeed;
            position.z += moveSpeed;
            break;
        case RIGHT:
            position.x += moveSpeed;
            break;
        case BACKWARD:
            position.z -= moveSpeed;
            break;
        case FORWARD:
            position.z += moveSpeed;
            break;
    }

}