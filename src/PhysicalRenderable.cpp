//
// Created by Engin Manap on 24.03.2016.
// Created by Engin Manap on 24.03.2016.
//

#include "PhysicalRenderable.h"


void PhysicalRenderable::updateTransformFromPhysics() {
    btTransform trans;
    rigidBody->getMotionState()->getWorldTransform(trans);

    transformation.setTransformationsNotPropagate(GLMConverter::BltToGLM(trans.getOrigin()), GLMConverter::BltToGLM(trans.getRotation()));
    updateAABB();

}
