//
// Created by engin on 15.02.2018.
//

#ifndef LIMONENGINE_PLAYER_H
#define LIMONENGINE_PLAYER_H

#include <string>
#include <iostream>
#include "../GameObject.h"
#include "limonAPI/InputStates.h"
#include "limonAPI/Options.h"
#include "limonAPI/PlayerExtensionInterface.h"
#include "LimonAPI/CameraAttachment.h"

class btDiscreteDynamicsWorld;
class GUIRenderable;

class Player : public GameObject, public CameraAttachment {
public:
    enum DebugModes { DEBUG_ENABLED, DEBUG_DISABLED, DEBUG_NOCHANGE };
    struct WorldSettings {
        DebugModes debugMode = DEBUG_DISABLED;
        bool audioPlaying = true;
        bool worldSimulation = true;
        bool editorShown = false;
        bool cursorFree = false;
        bool resetAnimations = false;
        bool menuInteraction  = false;
    };
protected:
    GUIRenderable* cursor = nullptr;
    WorldSettings worldSettings;
    OptionsUtil::Options *options = nullptr;
    PlayerExtensionInterface* playerExtension = nullptr;

    OptionsUtil::Options::Option<LimonTypes::Vec4> moveSpeedOption;
    OptionsUtil::Options::Option<LimonTypes::Vec4> walkSpeedOption;
    OptionsUtil::Options::Option<LimonTypes::Vec4> runSpeedOption;
    OptionsUtil::Options::Option<double> jumpFactorOption;
    OptionsUtil::Options::Option<double> lookAroundSpeedOption;

    bool dead = false;
public:
    enum moveDirections {
        NONE, FORWARD, BACKWARD, LEFT, RIGHT, LEFT_FORWARD, RIGHT_FORWARD, LEFT_BACKWARD, RIGHT_BACKWARD, UP
    };

    Player(GUIRenderable *cursor, OptionsUtil::Options *options, const glm::vec3 &position [[gnu::unused]], const glm::vec3 &lookDirection [[gnu::unused]])
            : cursor(cursor), options(options){

        moveSpeedOption = options->getOption<LimonTypes::Vec4>(HASH("moveSpeed"));
        jumpFactorOption = options->getOption<double>(HASH("jumpFactor"));
        walkSpeedOption = options->getOption<LimonTypes::Vec4>(HASH("walkSpeed"));
        runSpeedOption = options->getOption<LimonTypes::Vec4>(HASH("runSpeed"));
        lookAroundSpeedOption = options->getOption<double>(HASH("lookAroundSpeed"));
    };

    ~Player() override = default;

    virtual void move(moveDirections) = 0;

    virtual void rotate(float xPosition, float yPosition, float xChange, float yChange) = 0;

    virtual glm::vec3 getPosition() const = 0;

    /**
     * This method should be used when player type changes, like from physical to free fly etc.
     *
     * @param position
     * @param lookDirection
     */
    virtual void ownControl(const glm::vec3 &position, const glm::vec3 &lookDirection) = 0;

    virtual void registerToPhysicalWorld(btDiscreteDynamicsWorld *world [[gnu::unused]],
                                            int collisionGroup [[gnu::unused]],
                                            int collisionMaskForSelf [[gnu::unused]],
                                            int collisionMaskForGround [[gnu::unused]],
                                            const glm::vec3 &worldAABBMin [[gnu::unused]],
                                            const glm::vec3 &worldAABBMax [[gnu::unused]]) {}

    CameraAttachment * cameraAttachment = nullptr;

    void setCameraOverride(CameraAttachment * camera_attachment) {
        this->cameraAttachment = camera_attachment;
    }

    virtual void processPhysicsWorld(const btDiscreteDynamicsWorld *world [[gnu::unused]]) {};

    virtual glm::vec3 getLookDirection() const = 0;

    virtual void getWhereCameraLooks(glm::vec3 &fromPosition, glm::vec3 &lookDirection) const = 0;

    const WorldSettings& getWorldSettings() const {
        return this->worldSettings;
    }

    CameraAttachment* getCameraAttachment() {
        if(this->cameraAttachment != nullptr) {
            return this->cameraAttachment;
        }
        return this;
    }

    /************Game Object methods **************/
    uint32_t getWorldObjectID() const override {
        std::cerr << "Player doesn't have a world object ID, it shouldn't have been needed." << std::endl;
        return 0;
    }

    ObjectTypes getTypeID() const override {
        return ObjectTypes::PLAYER;
    };

    std::string getName() const override {
        return "Player";//Players doesn't have specific names
    };

    /************Game Object methods **************/
    virtual void processInput(const InputStates &inputState, long time [[gnu::unused]]) {
        float xPosition, yPosition, xChange, yChange;
        if (inputState.getMouseChange(xPosition, yPosition, xChange, yChange)) {
            rotate(xPosition, yPosition, xChange, yChange);
        }

        if (inputState.getInputEvents(InputStates::Inputs::RUN)) {
            LimonTypes::Vec4 movementSpeed;
            if(inputState.getInputStatus(InputStates::Inputs::RUN)) {
                movementSpeed = runSpeedOption.get();
                moveSpeedOption.set(movementSpeed);
            } else {
                movementSpeed = walkSpeedOption.get();
                moveSpeedOption.set(movementSpeed);
            }
        }

        Player::moveDirections direction = Player::NONE;
        //ignore if both are pressed.
        if (inputState.getInputStatus(InputStates::Inputs::MOVE_FORWARD) !=
            inputState.getInputStatus(InputStates::Inputs::MOVE_BACKWARD)) {
            if (inputState.getInputStatus(InputStates::Inputs::MOVE_FORWARD)) {
                direction = Player::FORWARD;
            } else {
                direction = Player::BACKWARD;
            }
        }
        if (inputState.getInputStatus(InputStates::Inputs::MOVE_LEFT) != inputState.getInputStatus(InputStates::Inputs::MOVE_RIGHT)) {
            if (inputState.getInputStatus(InputStates::Inputs::MOVE_LEFT)) {
                if (direction == Player::FORWARD) {
                    direction = Player::LEFT_FORWARD;
                } else if (direction == Player::BACKWARD) {
                    direction = Player::LEFT_BACKWARD;
                } else {
                    direction = Player::LEFT;
                }
            } else if (direction == Player::FORWARD) {
                direction = Player::RIGHT_FORWARD;
            } else if (direction == Player::BACKWARD) {
                direction = Player::RIGHT_BACKWARD;
            } else {
                direction = Player::RIGHT;
            }
        }

        if (inputState.getInputStatus(InputStates::Inputs::JUMP) && inputState.getInputEvents(InputStates::Inputs::JUMP)) {
            direction = Player::UP;
        }

        //if none, camera should handle how to get slower.
        move(direction);

    }

    PlayerExtensionInterface *getPlayerExtension() const {
        return playerExtension;
    }

    void setPlayerExtension(PlayerExtensionInterface *playerExtension) {
        delete this->playerExtension;
        this->playerExtension = playerExtension;
    }

    virtual void setDead() {};

    bool isDead() const {
        return dead;
    }

};


#endif //LIMONENGINE_PLAYER_H
