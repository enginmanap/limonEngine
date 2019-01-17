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
#include "API/LimonAPI.h"
#include "API/TriggerInterface.h"
#include "../BulletDebugDrawer.h"

class TriggerObject : public GameObject {
    std::string name;
    Transformation transformation;
    std::vector<LimonAPI::ParameterRequest> firstEnterParameters;
    std::vector<LimonAPI::ParameterRequest> enterParameters;
    std::vector<LimonAPI::ParameterRequest> exitParameters;

    uint32_t objectID;
    LimonAPI* limonAPI;

    TriggerInterface* firstEnterTriggerCode = nullptr;
    TriggerInterface* enterTriggerCode = nullptr;
    TriggerInterface* exitTriggerCode = nullptr;

    bool triggered = false;
    bool inside = false;
    bool enabledAny = false;
    bool enabledFirstTrigger = false;
    bool enabledEnterTrigger = false;
    bool enabledExitTrigger = false;

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

    static void PutTriggerInGui(LimonAPI *limonAPI, TriggerInterface *&triggerCode, std::vector<LimonAPI::ParameterRequest> &parameters,
                                    bool &enabled, uint32_t index);

    TriggerObject(uint32_t id, LimonAPI* limonAPI): objectID(id), limonAPI(limonAPI) {
        ghostObject->setCollisionShape(ghostShape);
        ghostObject->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
        ghostObject->setWorldTransform(btTransform(btQuaternion::getIdentity(), btVector3()));
        ghostObject->setUserPointer(static_cast<GameObject *>(this));
        name  = "TRIGGER-" + std::to_string(objectID);

        transformation.setUpdateCallback(std::bind(&TriggerObject::updatePhysicsFromTransform, this));
    }

    ~TriggerObject() {
        delete ghostObject;
        delete ghostShape;
        delete this->firstEnterTriggerCode;
        delete this->enterTriggerCode;
        delete this->exitTriggerCode;
    }

    btPairCachingGhostObject *getGhostObject() const {
        return ghostObject;
    }

    Transformation* getTransformation() {
        return &transformation;
    }

    void render(BulletDebugDrawer *debugDrawer);


    bool checkAndTrigger() {
        if(!enabledAny) {
            return false;
        }

        if(enterTriggerCode == nullptr && exitTriggerCode == nullptr && triggered) {
            return false;// first trigger done, and no other triggers found
        }
        bool playerFound = false;
        //Bullet collision callbacks are global, and since player is suppose to collide with world all the time, it doesn't make sense to use them
        for(int i = 0; i < ghostObject->getNumOverlappingObjects(); i++ ) {
            btCollisionObject* object = ghostObject->getOverlappingPairs().at(i);
            if(object->getUserPointer() != nullptr && static_cast<GameObject*>(object->getUserPointer())->getTypeID() == GameObject::PLAYER) {
                playerFound = true;
                break;
            }
        }

        /**
         * now decide what to do. 4 options
         * inside == found -> do nothing
         * found true, inside false -> if triggered do enter, if not and has first, do first
         * found false inside true -> do exit
         */

        if(inside == playerFound) {//means player was inside, and still is.
            return false;
        }
        if(playerFound) {
            inside = true;//we can assume inside was false in this if

            if(!triggered && firstEnterTriggerCode != nullptr) {
                this->triggered = true;
                return this->firstEnterTriggerCode->run(firstEnterParameters);
            }
            //now we are sure first is not called, either because it was before, or because
            // first is not defined, both cases, call enter trigger
            if(enterTriggerCode != nullptr) {
                this->triggered = true;
                return this->enterTriggerCode->run(enterParameters);
            }
        } else {
            inside = false; //assume it was true
            if(this->exitTriggerCode != nullptr) {
                return this->exitTriggerCode->run(exitParameters);
            }
        }
        return false;//means required trigger code was null;
    }


    /************Game Object methods **************/
    uint32_t getWorldObjectID() const override {
        return objectID;
    }
    ObjectTypes getTypeID() const {
        return GameObject::TRIGGER;
    };

    std::string getName() const {
        return name;
    };

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request);
    /************Game Object methods **************/


    void serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *triggersNode) const;
    static TriggerObject *deserialize(tinyxml2::XMLElement *triggerNode, LimonAPI *limonAPI);

    std::vector<LimonAPI::ParameterRequest> getResultOfCode(uint32_t codeID);
};


#endif //LIMONENGINE_TRIGGEROBJECT_H
