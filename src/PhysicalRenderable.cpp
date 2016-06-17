//
// Created by Engin Manap on 24.03.2016.
// Created by Engin Manap on 24.03.2016.
//

#include "PhysicalRenderable.h"


void PhysicalRenderable::updateTransformFromPhysics() {
    btTransform trans;
    rigidBody->getMotionState()->getWorldTransform(trans);

    this->translate.x = trans.getOrigin().getX();
    this->translate.y = trans.getOrigin().getY();
    this->translate.z = trans.getOrigin().getZ();

    this->orientation = glm::quat(cos(trans.getRotation().getAngle() / 2),
                                  trans.getRotation().getAxis().getX() * sin(trans.getRotation().getAngle() / 2),
                                  trans.getRotation().getAxis().getY() * sin(trans.getRotation().getAngle() / 2),
                                  trans.getRotation().getAxis().getZ() * sin(trans.getRotation().getAngle() / 2));

    //std::cout << "the objects last position is" << this->translate.x <<","<< this->translate.y <<","<<this->translate.z << std::endl;
    isDirty = true;
}