//
// Created by Engin Manap on 24.03.2016.
//

#ifndef LIMONENGINE_PHYSICAL_H
#define LIMONENGINE_PHYSICAL_H


#include "Renderable.h"
#include "Attachable.h"
#include "Utils/GLMConverter.h"
#include <memory>

class PhysicalRenderable : public Renderable, public Attachable {
protected:
    glm::mat4 centerOffsetMatrix;
    glm::vec3 centerOffset;//init by list for constructor
    glm::vec3 aabbMax, aabbMin;
    float mass;//not const: can be switched between 0 (static) and >0 (dynamic) at runtime, which reloads the collision shape
    btRigidBody *rigidBody = nullptr;
    bool disconnected = false;
    const float NOT_SCALE_LIMIT = 0.01;
    bool isScaled = true;

public:
    explicit PhysicalRenderable(GraphicsInterface* graphicsWrapper, float mass, bool disconnected)
            : Renderable(graphicsWrapper), centerOffset(glm::vec3(0, 0, 0)), mass(mass), disconnected(disconnected) {
        transformation.setGenerateWorldTransform(std::bind(&PhysicalRenderable::processTransformForPyhsics, this));
        transformation.setUpdateCallback([this]() noexcept { onTransformUpdated(); });
    }

    void onTransformUpdated() noexcept override {
        updatePhysicsFromTransform();
    }

    // --- Attachable interface ---
    Transformation* getTransformation() override { return &transformation; }
    const Transformation* getTransformation() const override { return &transformation; }

    // Covariant override — callers that hold PhysicalRenderable* still get a typed pointer back.
    PhysicalRenderable* getParentObject() const override {
        return dynamic_cast<PhysicalRenderable*>(parentObject);
    }

    // --- Physics ---
    btRigidBody *getRigidBody() { return rigidBody; };

    bool isDisconnected() const {
        return disconnected;
    }
    void updatePhysicsFromTransform() noexcept {
        if (rigidBody == nullptr) return;
        // Force recompute: getTranslate() is stale if setParentTransform ran before setTransformations (e.g. during attachTo).
        transformation.getWorldTransform();
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

    glm::mat4 processTransformForPyhsics() {
        if(centerOffset.x == 0.0f && centerOffset.y == 0.0f && centerOffset.z == 0.0f) {
            return glm::translate(glm::mat4(1.0f), transformation.getTranslateSingle()) * glm::mat4_cast(transformation.getOrientationSingle()) *
                   glm::scale(glm::mat4(1.0f), transformation.getScaleSingle());
        } else {
            return glm::translate(glm::mat4(1.0f), transformation.getTranslateSingle()) *
                   glm::mat4_cast(transformation.getOrientationSingle()) *
                   glm::scale(glm::mat4(1.0f), transformation.getScaleSingle()) *
                   glm::translate(glm::mat4(1.0f), -1.0f * centerOffset);
        }

    }

    virtual void updateTransformFromPhysics();

    virtual void renderWithProgram(std::shared_ptr<GraphicsProgram> program, uint32_t lodLevel) = 0;

    float getMass() const {
        return mass;
    };

    /**
     * Stores the mass value only. It does NOT rebuild the collision shape or touch the physics world; the body's
     * collision shape must be reloaded separately (see Model::reloadPhysicsShape) while the body is removed from
     * the dynamics world. Switching between 0 and >0 changes the shape type, so this must be followed by a reload.
     */
    void setMassValue(float newMass) {
        this->mass = newMass;
    }

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

    // Redeclare as pure virtual so Model and ModelGroup must still implement it.
    virtual bool fillObjects(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *objectsNode) const = 0;

    void updateAABB() {
        btVector3 abMax, abMin;
        // Compute directly from shape + current transform instead of reading the broad-phase
        // cache, which is stale until dynamicsWorld->updateSingleAabb() runs (next stepSimulation).
        rigidBody->getCollisionShape()->getAabb(rigidBody->getWorldTransform(), abMin, abMax);
        this->aabbMin = GLMConverter::BltToGLM(abMin);
        this->aabbMax = GLMConverter::BltToGLM(abMax);
        this->dirtyForFrustum = true;
    }

    const glm::vec3 &getCenterOffset() const {
        return centerOffset;
    }

    virtual std::vector<std::shared_ptr<Material>> getMaterials() const = 0;
};


#endif //LIMONENGINE_PHYSICAL_H
