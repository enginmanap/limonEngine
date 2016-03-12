//
// Created by Engin Manap on 3.03.2016.
//

#include "Renderable.h"

Renderable::Renderable(GLHelper* glHelper):
    glHelper(glHelper), isDirty(true), scale(1.0f,1.0f,1.0f) {
}

void Renderable::generateWorldTransform()  {
    this->oldWorldTransform = this->worldTransform;
    this->worldTransform = glm::translate(glm::mat4(1.0f), translate) * glm::mat4_cast(orientation)* glm::scale(glm::mat4(1.0f),scale);
    isDirty=false;
}


void Renderable::updateTransformFromPhysics(){
    btTransform trans;
    rigidBody->getMotionState()->getWorldTransform(trans);

    this->translate.x = trans.getOrigin().getX();
    this->translate.y = trans.getOrigin().getY();
    this->translate.z = trans.getOrigin().getZ();

    this->orientation = glm::quat(cos(trans.getRotation().getAngle()/2),
                                                    trans.getRotation().getAxis().getX() * sin(trans.getRotation().getAngle()/2),
                                                    trans.getRotation().getAxis().getY() * sin(trans.getRotation().getAngle()/2),
                                                    trans.getRotation().getAxis().getZ() * sin(trans.getRotation().getAngle()/2));

    //std::cout << "the objects last position is" << this->translate.x <<","<< this->translate.y <<","<<this->translate.z << std::endl;
    isDirty = true;
}