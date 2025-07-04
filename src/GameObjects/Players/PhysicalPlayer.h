//
// Created by engin on 12.02.2018.
//

#ifndef LIMONENGINE_PHYSICALPLAYER_H
#define LIMONENGINE_PHYSICALPLAYER_H


#include <vector>
#include <memory>

#include "Player.h"
#include "limonAPI/Options.h"
#include "limonAPI/CameraAttachment.h"
#include "../../Utils/GLMConverter.h"
#include "../Sound.h"
#include "../Model.h"

static const int STEPPING_TEST_COUNT = 5;
static const float MINIMUM_CLIMP_NORMAL_Y = 0.707f; // cant climb more than 45 degrees

class LimonAPI;

class PhysicalPlayer : public Player {
    glm::vec3 center, up, right;
    glm::quat view;
    btVector3 inputMovementSpeed;
    btVector3 groundFrictionMovementSpeed; //this is for emulating ground friction
    float groundFrictionFactor = 2.0f;
    float movementSpeedFactor = 1000;
    bool movementSpeedFull = true;
    btVector3 slowDownFactor = btVector3(2.0f, 1.0f, 2.0f);

    btRigidBody *player;
    btGeneric6DofSpring2Constraint *spring;
    float springStandPoint;
    float startingHeight;
    int collisionGroup;
    int collisionMask;
    int collisionMaskGround;
    uint32_t worldID = 0;

    std::vector<btCollisionWorld::ClosestRayResultCallback> rayCallbackArray;
    btCollisionWorld::ClosestRayResultCallback horizontalRayCallback = btCollisionWorld::ClosestRayResultCallback(btVector3(), btVector3());
    btTransform worldTransformHolder;
    bool onAir;
    bool positionSet = false;
    std::shared_ptr<Sound> currentSound = nullptr;

    bool dirty;
    bool skipSpringByJump = false;
    bool whileJump = false;
    Model* attachedModel = nullptr;
    glm::vec3 attachedModelOffset = glm::vec3(0,0,0);

    glm::quat calculatePlayerRotation() const;

    static const float CAPSULE_HEIGHT;
    static const float CAPSULE_RADIUS;
    static const float STANDING_HEIGHT;

public:
    glm::vec3 getPosition() const override {
        return GLMConverter::BltToGLM(player->getCenterOfMassPosition()) + glm::vec3(0.0f, 1.0f, 0.0f);
    }

    void move(moveDirections) override;

    void rotate(float xPosition, float yPosition, float xChange, float yChange) override;

    btRigidBody* getRigidBody() const {
        return player;
    }

    /**
     * This method is used to render the placeholder in editor mode
     * all parameters are references.
     *
     * I should/could build a model here, but then it whould be harder to extract this class with the API
     */


    void getRenderProperties(std::string& assetPath, glm::vec3& scale) {
        assetPath = "./Engine/Models/Capsule/Capsule2.obj"; //since this file is required by the engine itself.
        scale = glm::vec3(1,1,1);
    }

    void registerToPhysicalWorld(btDiscreteDynamicsWorld *world, int collisionGroup, int collisionMaskForSelf,
                                     int collisionMaskForGround, const glm::vec3 &worldAABBMin [[gnu::unused]], const glm::vec3 &worldAABBMax [[gnu::unused]]) override;

    void processPhysicsWorld(const btDiscreteDynamicsWorld *world) override;

    bool isDirty() const override {
        return dirty;
    }

    void clearDirty() override {
        this->dirty = false;
    }

    void getCameraVariables(glm::vec3& position, glm::vec3 &center, glm::vec3& up, glm::vec3& right) override {
        position = GLMConverter::BltToGLM(this->getRigidBody()->getWorldTransform().getOrigin());
        position.y += 1.0f;//for putting the camera up portion of capsule
        center = this->center;
        up = this->up;
        right = this->right;
    };

    /**
     * This method requires the world, because it raytests for closest object below the camera.
     * This is required because single sided spring constrain automatically attaches to world itself,
     * and we need to calculate an equilibrium point.
     *
     * @param world
     * @return
     */
    btGeneric6DofSpring2Constraint *getSpring(float minY);

    glm::vec3 getLookDirection() const override {
        return this->center;
    };

    glm::quat getLookDirectionQuaternion() const {
        return calculatePlayerRotation();
    }

    void getWhereCameraLooks(glm::vec3 &fromPosition, glm::vec3 &lookDirection) const override {
        fromPosition = this->getPosition();
        lookDirection = this->center;
    }

    void ownControl(const glm::vec3& position, const glm::vec3 &lookDirection) override;

    PhysicalPlayer(uint32_t worldID, OptionsUtil::Options *options, GUIRenderable *cursor, const glm::vec3 &position,
                   const glm::vec3 &lookDirection, Model *attachedModel = nullptr);

    ~PhysicalPlayer() override {
        delete player;
        delete spring;
    }

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request) override;

    void setAttachedModelOffset(const glm::vec3 &attachedModelOffset);

    const glm::vec3& getAttachedModelOffset() const {
        return attachedModelOffset;
    }

    void setAttachedModel(Model *attachedModel);

   void setAttachedModelTransformation(Model *attachedModel) const {
        if(attachedModel != nullptr) {
            attachedModel->getTransformation()->setTranslate(GLMConverter::BltToGLM(getRigidBody()->getWorldTransform().getOrigin()) + glm::vec3(0,1,0)  + getLookDirectionQuaternion() * attachedModelOffset);
        }
    }

    void processInput(const InputStates &inputHandler, long time) override;

    void interact(LimonAPI *limonAPI, std::vector<LimonTypes::GenericParameter> &interactionData) override;

    void setDead() override;

    uint32_t getWorldObjectID() const override {
        return worldID;
    }

};


#endif //LIMONENGINE_PHYSICALPLAYER_H
