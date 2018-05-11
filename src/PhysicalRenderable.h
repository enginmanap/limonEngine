//
// Created by Engin Manap on 24.03.2016.
//

#ifndef LIMONENGINE_PHYSICAL_H
#define LIMONENGINE_PHYSICAL_H


#include "Renderable.h"
#include "Utils/GLMConverter.h"

class PhysicalRenderable : public Renderable {
protected:
    glm::mat4 centerOffsetMatrix;
    glm::vec3 centerOffset;
    glm::vec3 aabbMax, aabbMin;
    btRigidBody *rigidBody;

    const float mass;


public:
    explicit PhysicalRenderable(GLHelper *glHelper, float mass) : Renderable(glHelper), centerOffset(glm::vec3(0, 0, 0)), mass(mass) {
        transformation.setGenerateWorldTransform(std::bind(&PhysicalRenderable::processTransformForPyhsics, this));
        transformation.setUpdateCallback(std::bind(&PhysicalRenderable::updatePhysicsFromTransform, this));
    };

    btRigidBody *getRigidBody() { return rigidBody; };

    void updatePhysicsFromTransform() {
        //std::cout << "updatePhysicsFromTransform transform call " << std::endl;
        rigidBody->getCollisionShape()->setLocalScaling(btVector3(transformation.getScale().x, transformation.getScale().y, transformation.getScale().z));

        btTransform transform = this->rigidBody->getCenterOfMassTransform();
        transform.setOrigin(btVector3(transformation.getTranslate().x, transformation.getTranslate().y, transformation.getTranslate().z));
        transform.setRotation(GLMConverter::GLMToBlt(transformation.getOrientation()));
        this->rigidBody->setWorldTransform(transform);
        this->rigidBody->getMotionState()->setWorldTransform(transform);
        this->rigidBody->activate();
        updateAABB();
    }

    /**
     * If there were any change with transform, trigger this
     */
    glm::mat4 processTransformForPyhsics() {
        //std::cout << "processTransformForPyhsics transform call " << std::endl;

        //if animated, then the transform information will be updated according to bone transforms. Then we apply current center offset
        return glm::translate(glm::mat4(1.0f), transformation.getTranslate()) * glm::mat4_cast(transformation.getOrientation()) *
               glm::scale(glm::mat4(1.0f), transformation.getScale()) * glm::translate(glm::mat4(1.0f), -1.0f * centerOffset);

    }

    void updateTransformFromPhysics();

    virtual void renderWithProgram(GLSLProgram &program) = 0;

    float getMass() const {
        return mass;
    };

    const glm::vec3 &getAabbMax() const {
        return aabbMax;
    }

    const glm::vec3 &getAabbMin() const {
        return aabbMin;
    }

    virtual void fillObjects(tinyxml2::XMLDocument& document, tinyxml2::XMLElement * objectsNode) const = 0;

    void updateAABB() {
        btVector3 abMax, abMin;
        rigidBody->getAabb(abMin,abMax);
        this->aabbMin = GLMConverter::BltToGLM(abMin);
        this->aabbMax = GLMConverter::BltToGLM(abMax);
    }
};


#endif //LIMONENGINE_PHYSICAL_H
