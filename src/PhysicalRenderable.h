//
// Created by Engin Manap on 24.03.2016.
//

#ifndef UBERGAME_PHYSICAL_H
#define UBERGAME_PHYSICAL_H


#include "Renderable.h"

class PhysicalRenderable : public Renderable {
protected:
    btRigidBody* rigidBody;
public:
    PhysicalRenderable(GLHelper* glHelper): Renderable(glHelper){};
    btRigidBody* getRigidBody() { return rigidBody;};

    void addScale(const glm::vec3& scale){
        Renderable::addScale(scale);
        rigidBody->getCollisionShape()->setLocalScaling(btVector3(this->scale.x, this->scale.y, this->scale.z));

    }

    void addTranslate(const glm::vec3& translate){
        Renderable::addTranslate(translate);
        btTransform transform =  this->rigidBody->getCenterOfMassTransform();
        transform.setOrigin(btVector3(this->translate.x,this->translate.y,this->translate.z));
        this->rigidBody->setWorldTransform(transform);
        this->rigidBody->getMotionState()->setWorldTransform(transform);
    }

    void updateTransformFromPhysics();
    void setWorldTransform(const glm::mat4& transformMatrix) {
        std::cerr << "Physical Renderables can not be set manually, don't use setWorldTransform." << std::endl;
    }
};


#endif //UBERGAME_PHYSICAL_H
