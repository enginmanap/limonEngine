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

void Camera::rotate(float xChange, float yChange) {
    viewChange = glm::quat(cos(yChange * lookAroundSpeed / 2),
                           right.x * sin(yChange * lookAroundSpeed / 2),
                           right.y * sin(yChange * lookAroundSpeed / 2),
                           right.z * sin(yChange * lookAroundSpeed / 2));

    view = viewChange * view * glm::conjugate(viewChange);
    view = glm::normalize(view);

    viewChange = glm::quat(cos(xChange * lookAroundSpeed / 2),
                           up.x * sin(xChange * lookAroundSpeed / 2),
                           up.y * sin(xChange * lookAroundSpeed / 2),
                           up.z * sin(xChange * lookAroundSpeed / 2));
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
    this->dirty=true;
}