//
// Created by engin on 8.05.2018.
//

#ifndef LIMONENGINE_TRIGGEROBJECT_H
#define LIMONENGINE_TRIGGEROBJECT_H

#include <bullet/btBulletCollisionCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include "../Utils/GLMConverter.h"
#include "GameObject.h"

class TriggerObject : public GameObject {
    glm::vec3 scale, translate;
    glm::quat orientation;
    uint32_t objectID;

    btCollisionShape *ghostShape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
    btPairCachingGhostObject *ghostObject = new btPairCachingGhostObject();

public:

    TriggerObject(uint32_t id): objectID(id) {
        ghostObject->setCollisionShape(ghostShape);
        ghostObject->setCollisionFlags(ghostObject->getCollisionFlags() &
                                             btCollisionObject::CF_NO_CONTACT_RESPONSE);
        ghostObject->setWorldTransform(btTransform(btQuaternion::getIdentity(), btVector3()));

        ghostObject->setUserPointer(static_cast<GameObject *>(this));
/*
        dynamicsWorld->addCollisionObject(ghostObject, btBroadphaseProxy::SensorTrigger,
                                        btBroadphaseProxy::AllFilter & ~btBroadphaseProxy::SensorTrigger);
*/
    }

    void addScale(const glm::vec3 &scale) {
        ghostObject->getCollisionShape()->setLocalScaling(btVector3(this->scale.x, this->scale.y, this->scale.z));
    }

    void setScale(const glm::vec3 &scale) {
        ghostObject->getCollisionShape()->setLocalScaling(btVector3(this->scale.x, this->scale.y, this->scale.z));
    }

    const glm::vec3 getScale() const {
        return this->scale;
    }

    void addTranslate(const glm::vec3 &translate) {
        btTransform transform = this->ghostObject->getWorldTransform();
        transform.setOrigin(btVector3(this->translate.x, this->translate.y, this->translate.z));
        this->ghostObject->setWorldTransform(transform);
        this->translate += translate;
    }

    void setTranslate(const glm::vec3 &translate) {
        this->translate = translate;
        btTransform transform = this->ghostObject->getWorldTransform();
        transform.setOrigin(btVector3(this->translate.x, this->translate.y, this->translate.z));
        this->ghostObject->setWorldTransform(transform);
    }

    glm::vec3 getTranslate() const {
        return this->translate;
    }

    void setOrientation(const glm::quat &orientation) {
        btTransform transform = this->ghostObject->getWorldTransform();
        transform.setRotation(GLMConverter::GLMToBlt(this->orientation));
        this->ghostObject->setWorldTransform(transform);
    }

    void addOrientation(const glm::quat &orientation) {
        btTransform transform = this->ghostObject->getWorldTransform();
        btQuaternion rotation = transform.getRotation();
        rotation = rotation * GLMConverter::GLMToBlt(orientation);
        transform.setRotation(rotation);
        this->ghostObject->setWorldTransform(transform);
    }

    glm::quat getOrientation() const {
        return this->orientation;
    }

    btPairCachingGhostObject *getGhostObject() const {
        return ghostObject;
    }


    /************Game Object methods **************/
    uint32_t getWorldObjectID() {
        return objectID;
    }
    ObjectTypes getTypeID() const {
        return GameObject::TRIGGER;
    };

    std::string getName() const {
        return "TRIGGER-" + std::to_string(objectID);
    };

    ImGuiResult addImGuiEditorElements() ;
    /************Game Object methods **************/

};


#endif //LIMONENGINE_TRIGGEROBJECT_H
