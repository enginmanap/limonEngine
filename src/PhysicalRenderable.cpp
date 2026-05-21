//
// Created by Engin Manap on 24.03.2016.
// Created by Engin Manap on 24.03.2016.
//

#include "PhysicalRenderable.h"


void PhysicalRenderable::updateTransformFromPhysics() {
    btTransform trans;
    rigidBody->getMotionState()->getWorldTransform(trans);

    glm::vec3 worldPos = GLMConverter::BltToGLM(trans.getOrigin());
    glm::quat worldRot = GLMConverter::BltToGLM(trans.getRotation());

    const Transformation* parent = transformation.getParentTransform();
    if (parent != nullptr) {
        // Physics always works in world space; convert back to parent-local space so
        // that translateSingle/orientationSingle remain the local offset the editor expects.
        glm::mat4 localMatrix = glm::inverse(parent->getWorldTransform())
                                * (glm::translate(glm::mat4(1.0f), worldPos) * glm::mat4_cast(worldRot));
        glm::vec3 localPos, localScale, skew;
        glm::quat localRot;
        glm::vec4 perspective;
        glm::decompose(localMatrix, localScale, localRot, localPos, skew, perspective);
        transformation.setTransformationsNotPropagate(localPos, glm::normalize(localRot));
    } else {
        transformation.setTransformationsNotPropagate(worldPos, worldRot);
    }
    //ATTENTION if the transform propagates, then the change will be send to physics, then this method will be called, infinite loop
    this->updateAABB();
}