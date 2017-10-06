//
// Created by Engin Manap on 3.03.2016.
//

#include "Renderable.h"

Renderable::Renderable(GLHelper *glHelper) :
        glHelper(glHelper), renderProgram(NULL), isDirty(true), isRotated(false), scale(1.0f, 1.0f, 1.0f) {
}

void Renderable::generateWorldTransform() {
    this->oldWorldTransform = this->worldTransform;
    this->worldTransform = glm::translate(glm::mat4(1.0f), translate) * glm::mat4_cast(orientation) *
                           glm::scale(glm::mat4(1.0f), scale);
    isDirty = false;
}

void Renderable::setWorldTransform(const glm::mat4 &transformMatrix) {
    this->oldWorldTransform = this->worldTransform;
    this->worldTransform = transformMatrix;
    //these 2 values are not used afterwards
    glm::vec3 tempSkew;
    glm::vec4 tempPerspective;
    glm::decompose(transformMatrix, scale, orientation, translate, tempSkew, tempPerspective);
    this->isDirty = false;
}