//
// Created by engin on 15.02.2018.
//

#ifndef LIMONENGINE_PLAYER_H
#define LIMONENGINE_PLAYER_H

#include <glm/glm.hpp>
#include <string>
#include <iostream>
#include "../GameObject.h"
#include "../../API/InputStates.h"
#include "../../API/LimonAPI.h"
#include "../../Options.h"
#include "../../API/PlayerExtensionInterface.h"

class btDiscreteDynamicsWorld;
class GUIRenderable;
class CameraAttachment;


class Player : public GameObject {
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
    Options *options = nullptr;
    PlayerExtensionInterface* playerExtension = nullptr;
    bool dead = false;
public:
    enum moveDirections {
        NONE, FORWARD, BACKWARD, LEFT, RIGHT, LEFT_FORWARD, RIGHT_FORWARD, LEFT_BACKWARD, RIGHT_BACKWARD, UP
    };

    Player(GUIRenderable *cursor, Options *options, const glm::vec3 &position [[gnu::unused]], const glm::vec3 &lookDirection [[gnu::unused]])
            : cursor(cursor), options(options){};

    virtual ~Player() {}

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

    /************Game Object methods **************/
    virtual uint32_t getWorldObjectID() const override {
        std::cerr << "Player doesn't have a world object ID, it shouldn't have been needed." << std::endl;
        return 0;
    }

    ObjectTypes getTypeID() const {
        return GameObject::PLAYER;
    };

    std::string getName() const {
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
                options->getOption("runSpeed", movementSpeed);
                options->setOption("moveSpeed", movementSpeed);
            } else {
                options->getOption("walkSpeed", movementSpeed);
                options->setOption("moveSpeed", movementSpeed);
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

    bool isDead() {
        return dead;
    }

};


#endif //LIMONENGINE_PLAYER_H
