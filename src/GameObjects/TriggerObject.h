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
#include "../Transformation.h"
#include "../GamePlay/LimonAPI.h"
#include "../GamePlay/TriggerInterface.h"
#include "../BulletDebugDrawer.h"

class TriggerObject : public GameObject {
    Transformation transformation;
    uint32_t objectID;
    bool triggered = false;
    bool enabled = false;
    TriggerInterface* triggerCode;

    btCollisionShape *ghostShape = new btBoxShape(btVector3(1.0f, 1.0f, 1.0f));
    btPairCachingGhostObject *ghostObject = new btPairCachingGhostObject();

    void updatePhysicsFromTransform() {
        //std::cout << "updatePhysicsFromTransform transform call " << std::endl;
        ghostObject->getCollisionShape()->setLocalScaling(btVector3(transformation.getScale().x, transformation.getScale().y, transformation.getScale().z));

        btTransform transform = ghostObject->getWorldTransform();
        transform.setOrigin(btVector3(transformation.getTranslate().x, transformation.getTranslate().y, transformation.getTranslate().z));
        transform.setRotation(GLMConverter::GLMToBlt(transformation.getOrientation()));
        ghostObject->setWorldTransform(transform);
    }

public:

    TriggerObject(uint32_t id, TriggerInterface* triggerCode): objectID(id), triggerCode(triggerCode) {
        ghostObject->setCollisionShape(ghostShape);
        ghostObject->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
        ghostObject->setWorldTransform(btTransform(btQuaternion::getIdentity(), btVector3()));
        ghostObject->setUserPointer(static_cast<GameObject *>(this));

        transformation.setUpdateCallback(std::bind(&TriggerObject::updatePhysicsFromTransform, this));

    }

    btPairCachingGhostObject *getGhostObject() const {
        return ghostObject;
    }

    Transformation* getTransformation() {
        return &transformation;
    }

    void render(BulletDebugDrawer *debugDrawer) {
        //render 12 lines

        glm::mat4 boxTransform = transformation.getWorldTransform();
        /* There are 8 points.
         * xyz
         * 1 +++
         * 2 ++-
         * 3 -++
         * 4 -+-
         *
         * 5 +-+
         * 6 +--
         * 7 --+
         * 8 ---
         * */

        //top
        debugDrawer->drawLine(boxTransform* glm::vec4( 1, 1, 1,1), boxTransform* glm::vec4( 1, 1,-1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 1 -> 2
        debugDrawer->drawLine(boxTransform* glm::vec4( 1, 1,-1,1), boxTransform* glm::vec4(-1, 1,-1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 2 -> 4
        debugDrawer->drawLine(boxTransform* glm::vec4(-1, 1,-1,1), boxTransform* glm::vec4(-1, 1, 1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 4 -> 3
        debugDrawer->drawLine(boxTransform* glm::vec4(-1, 1, 1,1), boxTransform* glm::vec4( 1, 1, 1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 3 -> 1

        //bottom
        debugDrawer->drawLine(boxTransform* glm::vec4( 1,-1, 1,1), boxTransform* glm::vec4( 1,-1,-1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 1 -> 2
        debugDrawer->drawLine(boxTransform* glm::vec4( 1,-1,-1,1), boxTransform* glm::vec4(-1,-1,-1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 2 -> 4
        debugDrawer->drawLine(boxTransform* glm::vec4(-1,-1,-1,1), boxTransform* glm::vec4(-1,-1, 1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 4 -> 3
        debugDrawer->drawLine(boxTransform* glm::vec4(-1,-1, 1,1), boxTransform* glm::vec4( 1,-1, 1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 3 -> 1

        //sides
        debugDrawer->drawLine(boxTransform* glm::vec4( 1, 1, 1,1), boxTransform* glm::vec4( 1,-1, 1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 1 -> 1
        debugDrawer->drawLine(boxTransform* glm::vec4( 1, 1,-1,1), boxTransform* glm::vec4( 1,-1,-1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 2 -> 2
        debugDrawer->drawLine(boxTransform* glm::vec4(-1, 1, 1,1), boxTransform* glm::vec4(-1,-1, 1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 3 -> 3
        debugDrawer->drawLine(boxTransform* glm::vec4(-1, 1,-1,1), boxTransform* glm::vec4(-1,-1,-1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 4 -> 4
    }

    bool checkAndTrigger() {
        if(triggered || !enabled) {
            return false;
        }
        //Bullet collision callbacks are global, and since player is suppose to collide with world all the time, it doesn't make sense to use them
        for(int i = 0; i < ghostObject->getNumOverlappingObjects(); i++ ) {
            btCollisionObject* object = ghostObject->getOverlappingPairs().at(i);
            if(object->getUserPointer() != nullptr && static_cast<GameObject*>(object->getUserPointer())->getTypeID() == GameObject::PLAYER) {
                triggered = true;
                return this->triggerCode->run();
            }
        }
        return false;
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

    GameObject::ImGuiResult addImGuiEditorElements(const glm::mat4 &cameraMatrix, const glm::mat4 &perspectiveMatrix);
    /************Game Object methods **************/


};


#endif //LIMONENGINE_TRIGGEROBJECT_H
