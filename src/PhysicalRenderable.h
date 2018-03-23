//
// Created by Engin Manap on 24.03.2016.
//

#ifndef LIMONENGINE_PHYSICAL_H
#define LIMONENGINE_PHYSICAL_H


#include "Renderable.h"
#include "Utils/GLMConverter.h"

class PhysicalRenderable : public Renderable {
protected:
    btRigidBody *rigidBody;
    glm::vec3 centerOffset;
public:
    explicit PhysicalRenderable(GLHelper *glHelper) : Renderable(glHelper), centerOffset(glm::vec3(0, 0, 0)) { };

    btRigidBody *getRigidBody() { return rigidBody; };

    void addScale(const glm::vec3 &scale) {
        Renderable::addScale(scale);
        rigidBody->getCollisionShape()->setLocalScaling(btVector3(this->scale.x, this->scale.y, this->scale.z));
        //local scale scales the shape, what about the rigid body -> shape positions
        btCompoundShape * shape = static_cast<btCompoundShape *>(rigidBody->getCollisionShape());
        for (int i = 0; i < shape->getNumChildShapes(); ++i) {
            btTransform transform = shape->getChildTransform(i);
            transform.setOrigin(GLMConverter::GLMToBlt(this->scale) * transform.getOrigin());
            shape->updateChildTransform(i, transform);
        }
        centerOffset = centerOffset * this->scale;
    }

    void setScale(const glm::vec3 &scale) {
        Renderable::setScale(scale);
        rigidBody->getCollisionShape()->setLocalScaling(btVector3(this->scale.x, this->scale.y, this->scale.z));
        //local scale scales the shape, what about the rigid body -> shape positions
        btCompoundShape * shape = static_cast<btCompoundShape *>(rigidBody->getCollisionShape());
        for (int i = 0; i < shape->getNumChildShapes(); ++i) {
            btTransform transform = shape->getChildTransform(i);
            transform.setOrigin(GLMConverter::GLMToBlt(scale) * transform.getOrigin());
            shape->updateChildTransform(i, transform);
        }
        centerOffset = centerOffset * scale;
    }

    void addTranslate(const glm::vec3 &translate) {
        Renderable::addTranslate(translate);
        btTransform transform = this->rigidBody->getCenterOfMassTransform();
        transform.setOrigin(btVector3(this->translate.x, this->translate.y, this->translate.z));
        this->rigidBody->setWorldTransform(transform);
        this->rigidBody->getMotionState()->setWorldTransform(transform);
    }

    void setTranslate(const glm::vec3 &translate) {
        Renderable::setTranslate(translate);
        btTransform transform = this->rigidBody->getCenterOfMassTransform();
        transform.setOrigin(btVector3(this->translate.x, this->translate.y, this->translate.z));
        this->rigidBody->setWorldTransform(transform);
        this->rigidBody->getMotionState()->setWorldTransform(transform);
    }

    void setOrientation(const glm::quat &orientation) {
        Renderable::setOrientation(orientation);
        btTransform transform = this->rigidBody->getCenterOfMassTransform();
        transform.setRotation(GLMConverter::GLMToBlt(this->orientation));
        this->rigidBody->setWorldTransform(transform);
        this->rigidBody->getMotionState()->setWorldTransform(transform);
    }

    void addOrientation(const glm::quat &orientation) {
        Renderable::addOrientation(orientation);
        btTransform transform = this->rigidBody->getCenterOfMassTransform();
        btQuaternion rotation = transform.getRotation();
        rotation = rotation * GLMConverter::GLMToBlt(orientation);
        transform.setRotation(rotation);
        this->rigidBody->setWorldTransform(transform);
        this->rigidBody->getMotionState()->setWorldTransform(transform);
    }

    void updateTransformFromPhysics();

    virtual void renderWithProgram(GLSLProgram &program) = 0;

};


#endif //LIMONENGINE_PHYSICAL_H
