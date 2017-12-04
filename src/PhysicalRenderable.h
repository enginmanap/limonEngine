//
// Created by Engin Manap on 24.03.2016.
//

#ifndef UBERGAME_PHYSICAL_H
#define UBERGAME_PHYSICAL_H


#include "Renderable.h"
#include "Utils/GLMConverter.h"

class PhysicalRenderable : public Renderable {
protected:
    btRigidBody *rigidBody;
    std::string objectType = "physicalRenderable";//FIXME this is just temporary ray test result detection code, we should return game objects instead of string
    glm::vec3 centerOffset;
public:
    PhysicalRenderable(GLHelper *glHelper) : Renderable(glHelper), centerOffset(glm::vec3(0, 0, 0)) { };

    btRigidBody *getRigidBody() { return rigidBody; };

    void addScale(const glm::vec3 &scale) {
        Renderable::addScale(scale);
        rigidBody->getCollisionShape()->setLocalScaling(btVector3(this->scale.x, this->scale.y, this->scale.z));

    }

    void addTranslate(const glm::vec3 &translate) {
        Renderable::addTranslate(translate);
        btTransform transform = this->rigidBody->getCenterOfMassTransform();
        transform.setOrigin(btVector3(this->translate.x, this->translate.y, this->translate.z));
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

    void setWorldTransform(const glm::mat4 &transformMatrix) {
        std::cerr << "Physical Renderables can not be set manually, don't use setWorldTransform." << std::endl;
    }

    void virtual renderWithProgram(GLSLProgram &program) = 0;

    void virtual setupForTime(long time) = 0;
};


#endif //UBERGAME_PHYSICAL_H
