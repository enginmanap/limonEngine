//
// Created by engin on 15.02.2018.
//

#ifndef LIMONENGINE_PLAYER_H
#define LIMONENGINE_PLAYER_H

class btDiscreteDynamicsWorld;

class Player {
public:
    enum moveDirections {
        NONE, FORWARD, BACKWARD, LEFT, RIGHT, LEFT_FORWARD, RIGHT_FORWARD, LEFT_BACKWARD, RIGHT_BACKWARD, UP
    };

    virtual void move(moveDirections) = 0;

    virtual void rotate(float xChange, float yChange) = 0;

    virtual glm::vec3 getPosition() const = 0;

    virtual void registerToPhysicalWorld(btDiscreteDynamicsWorld* world, const glm::vec3& worldAABBMin, const glm::vec3& worldAABBMax) = 0;

    virtual ~Player() {}

    virtual void processPhysicsWorld(const btDiscreteDynamicsWorld *world) = 0;

    virtual void getWhereCameraLooks(glm::vec3 &fromPosition, glm::vec3 &lookDirection) const = 0;
};


#endif //LIMONENGINE_PLAYER_H
