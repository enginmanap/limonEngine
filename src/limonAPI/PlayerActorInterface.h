//
// Created by engin on 23/02/2025.
//

#ifndef PLAYERACTORINTERFACE_H
#define PLAYERACTORINTERFACE_H

#include <string>
#include <iostream>
#include "limonAPI/InputStates.h"
#include "limonAPI/Options.h"
#include "limonAPI/PlayerExtensionInterface.h"

class btDiscreteDynamicsWorld;
class GUIRenderable;
class CameraAttachment;

class PlayerActorInterface {

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
    
    enum moveDirections {
        NONE, FORWARD, BACKWARD, LEFT, RIGHT, LEFT_FORWARD, RIGHT_FORWARD, LEFT_BACKWARD, RIGHT_BACKWARD, UP
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


    PlayerActorInterface(GUIRenderable *cursor, OptionsUtil::Options *options, const glm::vec3 &position [[gnu::unused]], const glm::vec3 &lookDirection [[gnu::unused]])
            : cursor(cursor), options(options){

        moveSpeedOption = options->getOption<LimonTypes::Vec4>(HASH("moveSpeed"));
        jumpFactorOption = options->getOption<double>(HASH("jumpFactor"));
        walkSpeedOption = options->getOption<LimonTypes::Vec4>(HASH("walkSpeed"));
        runSpeedOption = options->getOption<LimonTypes::Vec4>(HASH("runSpeed"));
        lookAroundSpeedOption = options->getOption<double>(HASH("lookAroundSpeed"));
    };

    ~PlayerActorInterface() = default;

    virtual void move(moveDirections) = 0;

    virtual void rotate(float xPosition, float yPosition, float xChange, float yChange) = 0;

    virtual glm::vec3 getPosition() const = 0;

    /**
     * This method should be used when player type changes, like from physical to free fly etc.
     *
     * @param position
     * @param lookDirection
     */
    virtual void ownControl(const glm::vec3 &position, const glm::vec3 lookDirection) = 0;

    virtual void registerToPhysicalWorld(btDiscreteDynamicsWorld *world, int collisionGroup, int collisionMaskForSelf,
                                             int collisionMaskForGround, const glm::vec3 &worldAABBMin, const glm::vec3 &worldAABBMax) = 0;

    virtual void processPhysicsWorld(const btDiscreteDynamicsWorld *world) = 0;

    virtual glm::vec3 getLookDirection() const = 0;

    virtual void getWhereCameraLooks(glm::vec3 &fromPosition, glm::vec3 &lookDirection) const = 0;

    const WorldSettings& getWorldSettings() const {
        return this->worldSettings;
    }

    virtual CameraAttachment* getCameraAttachment() = 0;

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

        PlayerActorInterface::moveDirections direction = PlayerActorInterface::NONE;
        //ignore if both are pressed.
        if (inputState.getInputStatus(InputStates::Inputs::MOVE_FORWARD) !=
            inputState.getInputStatus(InputStates::Inputs::MOVE_BACKWARD)) {
            if (inputState.getInputStatus(InputStates::Inputs::MOVE_FORWARD)) {
                direction = PlayerActorInterface::FORWARD;
            } else {
                direction = PlayerActorInterface::BACKWARD;
            }
        }
        if (inputState.getInputStatus(InputStates::Inputs::MOVE_LEFT) != inputState.getInputStatus(InputStates::Inputs::MOVE_RIGHT)) {
            if (inputState.getInputStatus(InputStates::Inputs::MOVE_LEFT)) {
                if (direction == PlayerActorInterface::FORWARD) {
                    direction = PlayerActorInterface::LEFT_FORWARD;
                } else if (direction == PlayerActorInterface::BACKWARD) {
                    direction = PlayerActorInterface::LEFT_BACKWARD;
                } else {
                    direction = PlayerActorInterface::LEFT;
                }
            } else if (direction == PlayerActorInterface::FORWARD) {
                direction = PlayerActorInterface::RIGHT_FORWARD;
            } else if (direction == PlayerActorInterface::BACKWARD) {
                direction = PlayerActorInterface::RIGHT_BACKWARD;
            } else {
                direction = PlayerActorInterface::RIGHT;
            }
        }

        if (inputState.getInputStatus(InputStates::Inputs::JUMP) && inputState.getInputEvents(InputStates::Inputs::JUMP)) {
            direction = PlayerActorInterface::UP;
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

    bool isDead() {
        return dead;
    }
    };

#endif //PLAYERACTORINTERFACE_H