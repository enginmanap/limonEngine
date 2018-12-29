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
#include <map>

#include "AIMovementNode.h"
#include "../Utils/GLMConverter.h"
#include "../Utils/GLMUtils.h"
#include "../BulletDebugDrawer.h"

//bigger than sqrt(3)/2
//avoiding sqrt
#define GRID_SNAP_DISTANCE (0.8f * 0.8f)

#define X_Z_DISTANCE 0.1
#define Y_DISTANCE_SQ 0.25

class AIMovementGrid {

    struct AINodeWithPriority {
        const AIMovementNode *node;
        float priority;

        AINodeWithPriority(const AIMovementNode *node, float priority) : node(node), priority(priority) {}

        bool operator>(const AINodeWithPriority &aiRight) const {
            return priority > aiRight.priority;
        }
    };

    bool inline isPositionCloseEnoughYOnly(const glm::vec3 &position1, const glm::vec3 &position2) const {
        return ((fabs(position1.x - position2.x) < X_Z_DISTANCE) &&
                (fabs(position1.z - position2.z) < X_Z_DISTANCE) &&
                (((position1.y - position2.y) * (position1.y - position2.y)) <= Y_DISTANCE_SQ));
    }

    bool inline isPositionCloseEnough(const glm::vec3 &position1, const glm::vec3 &position2) const {
        return (glm::length2(position1 - position2) < GRID_SNAP_DISTANCE);
    }

    AIMovementNode *root = nullptr;
    btCollisionShape *ghostShape = nullptr;
    btPairCachingGhostObject *sharedGhostObject = new btPairCachingGhostObject();
    btCollisionWorld::ClosestRayResultCallback *rayCallback = new btCollisionWorld::ClosestRayResultCallback(
            btVector3(0, 0, 0), btVector3(0, 0, 0));
    btManifoldArray sharedManifoldArray;
    std::map<int, const AIMovementNode *> actorLastNodeMap;
    uint32_t nextPossibleIndex = 1;//this is to be used internal and constructor only. Not thread safe

    int isThereCollisionCounter = 0;//this is only meaningful for debug
    float capsuleHeight = 1.30f + 0.1f;
    float capsuleRadius = 0.35f;//FIXME these should be configurable
    bool isThereCollision(btDiscreteDynamicsWorld *staticWorld);

    glm::vec3 max,min;
    std::vector<AIMovementNode *> visited;
    std::vector<AIMovementNode *> doneNodes;

    AIMovementNode *isAlreadyVisited(const glm::vec3 &position, size_t &indexOf);

    AIMovementNode *
    walkMonster(glm::vec3 walkPoint, btDiscreteDynamicsWorld *staticWorld, const glm::vec3 &min,
                    const glm::vec3 &max, uint32_t collisionGroup, uint32_t collisionMask);

    const AIMovementNode *
    aStarPath(const AIMovementNode *start, const glm::vec3 &destination, uint32_t maximumNumberOfNodes,
                  std::vector<glm::vec3> *route);

    uint32_t getNextID() {
        return nextPossibleIndex++;
    }

    AIMovementGrid() {};//used for deserialize

public:
    static constexpr float floatingHeight = 2.0f;

    AIMovementGrid(glm::vec3 startPoint, btDiscreteDynamicsWorld *staticOnlyPhysicsWorld, glm::vec3 min,
                       glm::vec3 max, uint32_t collisionGroup, uint32_t collisionMask);

    ~AIMovementGrid() {
        delete rayCallback;
        delete sharedGhostObject;
        delete ghostShape;
        for (unsigned int i = 0; i < visited.size(); ++i) {
            delete visited[i];
        }

        for (unsigned int i = 0; i < doneNodes.size(); ++i) {
            delete doneNodes[i];
        }

    }

    bool coursePath(const glm::vec3 &from, const glm::vec3 &to, uint32_t actorId, uint32_t maximumNumberOfNodes,
                    std::vector<glm::vec3> *route);

    bool coursePath(const glm::vec3 &from, const glm::vec3 &to, uint32_t maximumNumberOfNodes, std::vector<glm::vec3> *route);

    void debugDraw(BulletDebugDrawer *debugDrawer) const;

    bool setProperHeight(glm::vec3 *position, float floatingHeight, float checkHeight,
                         btDiscreteDynamicsWorld *staticWorld);

    bool serialize(const std::string& fileName);

    static AIMovementGrid* deserialize(const std::string& fileName);

    void
    serializeNode(tinyxml2::XMLDocument &aiGridDocument, tinyxml2::XMLNode *rootNode, const AIMovementNode *nodeToSerialize) const;
};


#endif //LIMONENGINE_AIMOVEMENTGRID_H
