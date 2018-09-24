//
// Created by engin on 15.02.2018.
//

#ifndef LIMONENGINE_PLAYER_H
#define LIMONENGINE_PLAYER_H

#include "../GameObject.h"
#include <glm/glm.hpp>
#include <string>
#include <iostream>

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
    GUIRenderable* cursor;
    WorldSettings worldSettings;
public:
    enum moveDirections {
        NONE, FORWARD, BACKWARD, LEFT, RIGHT, LEFT_FORWARD, RIGHT_FORWARD, LEFT_BACKWARD, RIGHT_BACKWARD, UP
    };

    Player(GUIRenderable *cursor, const glm::vec3 &position __attribute((unused)) , const glm::vec3 &lookDirection __attribute((unused))) : cursor(cursor){};

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

    virtual void registerToPhysicalWorld(btDiscreteDynamicsWorld *world, int collisionGroup, int collisionMask,
                                             const glm::vec3 &worldAABBMin, const glm::vec3 &worldAABBMax) = 0;

    virtual void processPhysicsWorld(const btDiscreteDynamicsWorld *world) = 0;

    virtual glm::vec3 getLookDirection() const = 0;

    virtual void getWhereCameraLooks(glm::vec3 &fromPosition, glm::vec3 &lookDirection) const = 0;

    const WorldSettings& getWorldSettings() const {
        return this->worldSettings;
    }

    virtual CameraAttachment* getCameraAttachment() = 0;

    /************Game Object methods **************/
    uint32_t getWorldObjectID() {
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
};


#endif //LIMONENGINE_PLAYER_H
