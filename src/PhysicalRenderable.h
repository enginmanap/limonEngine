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
    explicit PhysicalRenderable(GLHelper *glHelper, float mass) : Renderable(glHelper), centerOffset(glm::vec3(0, 0, 0)), mass(mass) { };

    btRigidBody *getRigidBody() { return rigidBody; };

    void addScale(const glm::vec3 &scale) {
        Renderable::addScale(scale);
        rigidBody->getCollisionShape()->setLocalScaling(btVector3(this->scale.x, this->scale.y, this->scale.z));
        updateAABB();
    }

    void setScale(const glm::vec3 &scale) {
        Renderable::setScale(scale);
        rigidBody->getCollisionShape()->setLocalScaling(btVector3(this->scale.x, this->scale.y, this->scale.z));
        updateAABB();
    }

    const glm::vec3 getScale() const {
        return this->scale;
    }

    void addTranslate(const glm::vec3 &translate) {
        Renderable::addTranslate(translate);
        btTransform transform = this->rigidBody->getCenterOfMassTransform();
        transform.setOrigin(btVector3(this->translate.x, this->translate.y, this->translate.z));
        this->rigidBody->setWorldTransform(transform);
        this->rigidBody->getMotionState()->setWorldTransform(transform);
        updateAABB();
    }

    void setTranslate(const glm::vec3 &translate) {
        Renderable::setTranslate(translate);
        btTransform transform = this->rigidBody->getCenterOfMassTransform();
        transform.setOrigin(btVector3(this->translate.x, this->translate.y, this->translate.z));
        this->rigidBody->setWorldTransform(transform);
        this->rigidBody->getMotionState()->setWorldTransform(transform);
        updateAABB();
    }

    glm::vec3 getTranslate() const {
        return this->translate;
    }

    void setOrientation(const glm::quat &orientation) {
        Renderable::setOrientation(orientation);
        btTransform transform = this->rigidBody->getCenterOfMassTransform();
        transform.setRotation(GLMConverter::GLMToBlt(this->orientation));
        this->rigidBody->setWorldTransform(transform);
        this->rigidBody->getMotionState()->setWorldTransform(transform);
        updateAABB();
    }

    void addOrientation(const glm::quat &orientation) {
        Renderable::addOrientation(orientation);
        btTransform transform = this->rigidBody->getCenterOfMassTransform();
        btQuaternion rotation = transform.getRotation();
        rotation = rotation * GLMConverter::GLMToBlt(orientation);
        transform.setRotation(rotation);
        this->rigidBody->setWorldTransform(transform);
        this->rigidBody->getMotionState()->setWorldTransform(transform);
        updateAABB();
    }

    glm::quat getOrientation() const {
        return this->orientation;
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
