//
// Created by Engin Manap on 24.03.2016.
//

#ifndef LIMONENGINE_PHYSICAL_H
#define LIMONENGINE_PHYSICAL_H


#include "Renderable.h"
#include "Utils/GLMConverter.h"
#include "GameObjects/Sound.h"
#include <memory>

class PhysicalRenderable : public Renderable {
protected:
    glm::mat4 centerOffsetMatrix;
    glm::vec3 centerOffset;//init by list for constructor
    glm::vec3 aabbMax, aabbMin;
    const float mass;
    btRigidBody *rigidBody = nullptr;
    PhysicalRenderable* parentObject = nullptr; //this points to ModelGroup if this is part of a group.
    int32_t parentBoneID = -1;
    bool disconnected = false;
    std::vector<PhysicalRenderable*> children;
    std::unique_ptr<Sound> soundAttachment2 = nullptr;
    const float NOT_SCALE_LIMIT = 0.01;
    bool isScaled = true;

public:
    explicit PhysicalRenderable(GraphicsInterface* graphicsWrapper, float mass, bool disconnected)
            : Renderable(graphicsWrapper), centerOffset(glm::vec3(0, 0, 0)), mass(mass), disconnected(disconnected) {
        transformation.setGenerateWorldTransform(std::bind(&PhysicalRenderable::processTransformForPyhsics, this));
        transformation.setUpdateCallback(std::bind(&PhysicalRenderable::updatePhysicsFromTransform, this));
    };

    btRigidBody *getRigidBody() { return rigidBody; };

    bool isDisconnected() const {
        return disconnected;
    }
    void updatePhysicsFromTransform() {
        rigidBody->getCollisionShape()->setLocalScaling(btVector3(transformation.getScale().x, transformation.getScale().y, transformation.getScale().z));
        if( std::fabs(transformation.getScale().x - 1.0f) < NOT_SCALE_LIMIT &&
            std::fabs(transformation.getScale().y - 1.0f) < NOT_SCALE_LIMIT &&
            std::fabs(transformation.getScale().z - 1.0f) < NOT_SCALE_LIMIT) {
            this->isScaled = false;
        } else {
            this->isScaled = true;
        }
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
        //if animated, then the transform information will be updated according to bone transforms. Then we apply current center offset
        if(centerOffset.x == 0.0f && centerOffset.y == 0.0f && centerOffset.z == 0.0f) {
            return glm::translate(glm::mat4(1.0f), transformation.getTranslateSingle()) * glm::mat4_cast(transformation.getOrientationSingle()) *
                   glm::scale(glm::mat4(1.0f), transformation.getScaleSingle());
        } else {
            //difference is the center offset
            return glm::translate(glm::mat4(1.0f), transformation.getTranslateSingle()) *
                   glm::mat4_cast(transformation.getOrientationSingle()) *
                   glm::scale(glm::mat4(1.0f), transformation.getScaleSingle()) *
                   glm::translate(glm::mat4(1.0f), -1.0f * centerOffset);
        }

    }

    virtual void updateTransformFromPhysics();

    virtual void renderWithProgram(std::shared_ptr<GraphicsProgram> program) = 0;

    float getMass() const {
        return mass;
    };

    bool disconnectFromPhysicsWorld(btDiscreteDynamicsWorld *dynamicsWorld) {
        if(this->disconnected) {
            return false;
        }
        dynamicsWorld->removeRigidBody(this->rigidBody);
        this->disconnected = true;
        return true;
    }

    bool connectToPhysicsWorld(btDiscreteDynamicsWorld *dynamicsWorld, int collisionGroup, int collisionMask) {
        if(!this->disconnected) {
            return false;
        }
        dynamicsWorld->addRigidBody(this->rigidBody,collisionGroup, collisionMask);
        this->disconnected = false;
        return true;
    }

    const glm::vec3 &getAabbMax() const {
        return aabbMax;
    }

    const glm::vec3 &getAabbMin() const {
        return aabbMin;
    }

    virtual bool fillObjects(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *objectsNode) const = 0;

    void updateAABB() {
        btVector3 abMax, abMin;
        rigidBody->getAabb(abMin,abMax);
        this->aabbMin = GLMConverter::BltToGLM(abMin);
        this->aabbMax = GLMConverter::BltToGLM(abMax);
        this->dirtyForFrustum = true;

        if(this->soundAttachment2 != nullptr) {
            this->soundAttachment2->setWorldPosition(this->transformation.getTranslate());
        }
    }

    void setSoundAttachementAndPlay(std::unique_ptr<Sound> soundAttachment) {
        this->soundAttachment2 = std::move(soundAttachment);
        this->soundAttachment2->setLoop(true);
        this->soundAttachment2->setWorldPosition(this->transformation.getTranslate());
        this->soundAttachment2->play();
    }

    void detachSound() {
        this->soundAttachment2->stop();
        this->soundAttachment2.reset(nullptr);
    }

    PhysicalRenderable *getParentObject() const {
        return parentObject;

    }

    void setParentObject(PhysicalRenderable *parentObject, int32_t parentBoneID = -1){
        this->parentObject = parentObject;
        this->parentBoneID = parentBoneID;
    }

    virtual void addChild(PhysicalRenderable* otherModel) {
        this->children.push_back(otherModel);
    }

    virtual bool removeChild(PhysicalRenderable* otherModel) {
        for (auto iterator = children.begin();
             iterator != children.end(); ++iterator) {
            if((*iterator) == otherModel) {
                children.erase(iterator);
                return true;
            }
        }
        return false;
    }

    const std::vector<PhysicalRenderable*> &getChildren() const {
        return children;
    }

    bool hasChildren() const {
        return !children.empty();
    }

    const glm::vec3 &getCenterOffset() const {
        return centerOffset;
    }
};


#endif //LIMONENGINE_PHYSICAL_H
