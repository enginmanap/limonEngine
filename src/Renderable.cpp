//
// Created by Engin Manap on 3.03.2016.
//

#include "Renderable.h"

Renderable::Renderable(GLHelper *glHelper) :
        glHelper(glHelper), renderProgram(nullptr), scale(1.0f, 1.0f, 1.0f), isDirty(true), isRotated(false) {
}

void Renderable::generateWorldTransform() {
    this->oldWorldTransform = this->worldTransform;
    this->worldTransform = glm::translate(glm::mat4(1.0f), translate) * glm::mat4_cast(orientation) *
                           glm::scale(glm::mat4(1.0f), scale);
    isDirty = false;
}
