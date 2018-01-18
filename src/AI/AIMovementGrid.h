//
// Created by engin on 1.01.2018.
//

#ifndef LIMONENGINE_AIMOVEMENTGRID_H
#define LIMONENGINE_AIMOVEMENTGRID_H


#include <glm/vec3.hpp>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <iostream>
#include <vector>
#include <queue>
#include <unordered_set>

#include "AIMovementNode.h"
#include "../Utils/GLMConverter.h"
#include "../Utils/GLMUtils.h"
#include "../BulletDebugDrawer.h"

//bigger than sqrt(3)/2
#define GRID_SNAP_DISTANCE 0.75f

class AIMovementGrid {

    struct AINodeWithPriority {
        const AIMovementNode *node;
        float priority;

        AINodeWithPriority(const AIMovementNode *node, float priority) : node(node), priority(priority) {}

        bool operator>(const AINodeWithPriority &aiRight) const {
            return priority > aiRight.priority;
        }
    };

    bool inline isPositionCloseEnough(const glm::vec3 &position1, const glm::vec3 &position2) const {
        return (glm::length(position1 - position2) < GRID_SNAP_DISTANCE);
    }

    AIMovementNode *root = nullptr;
    btCollisionShape *ghostShape;
    btPairCachingGhostObject *sharedGhostObject = new btPairCachingGhostObject();
    btCollisionWorld::ClosestRayResultCallback *rayCallback = new btCollisionWorld::ClosestRayResultCallback(
            btVector3(0, 0, 0), btVector3(0, 0, 0));
    btManifoldArray sharedManifoldArray;
    std::map<int, const AIMovementNode *> actorLastNodeMap;

    int isThereCollisionCounter = 0;//this is only meaninful for debug
    float capsuleHeight = 0.5f;
    float capsuleRadius = 0.5f;//FIXME these should be configurable
    bool isThereCollision(btDiscreteDynamicsWorld *staticWorld);

    std::vector<AIMovementNode *> visited;

    AIMovementNode *isAlreadyVisited(const AIMovementNode *node);

    AIMovementNode *
    walkMonster(glm::vec3 walkPoint, btDiscreteDynamicsWorld *staticWorld, const glm::vec3 &min, const glm::vec3 &max);

    const AIMovementNode *
    aStarPath(const AIMovementNode *start, const glm::vec3 &destination, std::vector<glm::vec3> *route);

    std::vector<const AIMovementNode *> calculatedNodes;

public:
    AIMovementGrid(glm::vec3 startPoint, btDiscreteDynamicsWorld *staticOnlyPhysicsWorld, glm::vec3 min, glm::vec3 max);

    ~AIMovementGrid() {
        delete rayCallback;
        delete sharedGhostObject;
        delete ghostShape;
        for (int i = 0; i < visited.size(); ++i) {
            delete visited[i];
        }

    }

    bool coursePath(const glm::vec3 &from, const glm::vec3 &to, int actorId, std::vector<glm::vec3> *route);

    bool coursePath(const glm::vec3 &from, const glm::vec3 &to, std::vector<glm::vec3> *route);

    void debugDraw(BulletDebugDrawer *debugDrawer) const;

    bool setProperHeight(glm::vec3 *position, float floatingHeight, float checkHeight,
                         btDiscreteDynamicsWorld *staticWorld);
};


#endif //LIMONENGINE_AIMOVEMENTGRID_H
