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
#include <fstream>
#include <vector>
#include <queue>
#include <unordered_set>
#include <map>

#ifdef CEREAL_SUPPORT
#include <cereal/access.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include "../Utils/GLMCerealConverters.hpp"
#endif

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
        std::shared_ptr<const AIMovementNode> node;
        float priority;

        AINodeWithPriority(std::shared_ptr<const AIMovementNode> node, float priority) : node(node), priority(priority) {}

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

    std::shared_ptr<AIMovementNode> root = nullptr;
    btCollisionShape *ghostShape = nullptr;
    btPairCachingGhostObject *sharedGhostObject = new btPairCachingGhostObject();
    btCollisionWorld::ClosestRayResultCallback *rayCallback = new btCollisionWorld::ClosestRayResultCallback(
            btVector3(0, 0, 0), btVector3(0, 0, 0));
    btManifoldArray sharedManifoldArray;
    std::map<int, std::shared_ptr<const AIMovementNode>> actorLastNodeMap;
    uint32_t nextPossibleIndex = 1;//this is to be used internal and constructor only. Not thread safe

    int isThereCollisionCounter = 0;//this is only meaningful for debug
    float capsuleHeight = 1.30f + 0.1f;
    float capsuleRadius = 0.35f;//FIXME these should be configurable
    bool isThereCollision(btDiscreteDynamicsWorld *staticWorld);

    glm::vec3 max,min;
    std::vector<std::shared_ptr<AIMovementNode>> visited;
    std::vector<std::shared_ptr<AIMovementNode>> doneNodes;

    std::shared_ptr<AIMovementNode> isAlreadyVisited(const glm::vec3 &position, size_t &indexOf);

    std::shared_ptr<AIMovementNode>
    walkMonster(glm::vec3 walkPoint, btDiscreteDynamicsWorld *staticWorld, const glm::vec3 &min,
                    const glm::vec3 &max, uint32_t collisionGroup, uint32_t collisionMask);

    std::shared_ptr<const AIMovementNode>
    aStarPath(std::shared_ptr<const AIMovementNode>start, const glm::vec3 &destination, uint32_t maximumNumberOfNodes,
                  std::vector<glm::vec3> *route);

    uint32_t getNextID() {
        return nextPossibleIndex++;
    }

#ifdef CEREAL_SUPPORT
    friend class cereal::access;
#endif
    AIMovementGrid() {};//used for deserialize

public:
    static constexpr float floatingHeight = 2.0f;

    AIMovementGrid(glm::vec3 startPoint, btDiscreteDynamicsWorld *staticOnlyPhysicsWorld, glm::vec3 min,
                       glm::vec3 max, uint32_t collisionGroup, uint32_t collisionMask);

    ~AIMovementGrid() {
        delete rayCallback;
        delete sharedGhostObject;
        delete ghostShape;

    }

    bool coursePath(const glm::vec3 &from, const glm::vec3 &to, uint32_t actorId, uint32_t maximumNumberOfNodes,
                    std::vector<glm::vec3> *route);

    bool coursePath(const glm::vec3 &from, const glm::vec3 &to, uint32_t maximumNumberOfNodes, std::vector<glm::vec3> *route);

    void debugDraw(BulletDebugDrawer *debugDrawer) const;

    bool setProperHeight(glm::vec3 *position, float floatingHeight, float checkHeight,
                         btDiscreteDynamicsWorld *staticWorld);

    bool serializeXML(const std::string& fileName);

    static AIMovementGrid* deserialize(const std::string& fileName);

    void
    serializeNode(tinyxml2::XMLDocument &aiGridDocument, tinyxml2::XMLNode *rootNode, std::shared_ptr<const AIMovementNode> nodeToSerialize) const;

#ifdef CEREAL_SUPPORT
    struct FlattenedNode {
        glm::vec3 position;
        int neigbours[9] = {0};//4 is self
        bool movable;

        template<class Archive>
        void serialize(Archive & archive){
            archive(position, neigbours, movable);
        }
    };


    template<class Archive>
    void save(Archive& archive) const {
        //build flattenedNode array
        std::vector<FlattenedNode> saveNodes;
        saveNodes.resize(this->nextPossibleIndex);
        for (size_t i = 0; i < doneNodes.size(); ++i) {
            saveNodes[doneNodes[i]->getID()].position = doneNodes[i]->getPosition();
            saveNodes[doneNodes[i]->getID()].movable = doneNodes[i]->isIsMovable();
            for (int j = 0; j < 9; ++j) {
                if(doneNodes[i]->getNeighbour(j) == nullptr) {
                    saveNodes[doneNodes[i]->getID()].neigbours[j] = 0;
                } else {
                    saveNodes[doneNodes[i]->getID()].neigbours[j] = doneNodes[i]->getNeighbour(j)->getID();
                }            }
            saveNodes[doneNodes[i]->getID()].neigbours[4] = doneNodes[i]->getID();//self ID at 4
        }

        for (size_t i = 0; i < visited.size(); ++i) {
            saveNodes[visited[i]->getID()].position = visited[i]->getPosition();
            saveNodes[visited[i]->getID()].movable = visited[i]->isIsMovable();
            for (int j = 0; j < 9; ++j) {
                if(visited[i]->getNeighbour(j) == nullptr) {
                    saveNodes[visited[i]->getID()].neigbours[j] = 0;
                } else {
                    saveNodes[visited[i]->getID()].neigbours[j] = visited[i]->getNeighbour(j)->getID();
                }
            }
            saveNodes[visited[i]->getID()].neigbours[4] = visited[i]->getID();//self ID at 4
        }

        archive(nextPossibleIndex, saveNodes);
    }

    template<class Archive>
    void load(Archive& archive) {
        std::vector<FlattenedNode> saveNodes;
        archive(nextPossibleIndex, saveNodes);

        this->doneNodes.resize(nextPossibleIndex);
        for (uint32_t i = 0; i < nextPossibleIndex; ++i) {
            doneNodes[i] = std::make_shared<AIMovementNode>(0, glm::vec3(0,100,0));//we are creating nodes empty, so we can link them together later
        }

        //now unflatten the nodes and move to doneNodes
        for (size_t i = 0; i < saveNodes.size(); ++i) {
            int nodeID = saveNodes[i].neigbours[4];
            doneNodes[nodeID]->setPosition(saveNodes[i].position);
            doneNodes[nodeID]->setIsMovable(saveNodes[i].movable);
            for (int j = 0; j < 9; ++j) {
                if(j == 4) {
                    continue;
                }
                if(saveNodes[i].neigbours[j] != 0 ) {
                    doneNodes[nodeID]->setNeighbour(j, doneNodes[saveNodes[i].neigbours[j]]);
                } else {
                    doneNodes[nodeID]->setNeighbour(j, nullptr);
                }
            }
        }
        if(doneNodes.size() > 1) {
            root = doneNodes[1];
        }

    }

    static AIMovementGrid* deserializeBinary(const std::string& fileName) {
        std::ifstream is(fileName, std::ios::binary);
        if( is.fail()) {
            std::cout << "Binary AI walk grid read failed. Fallback to XML read" << std::endl;
            return nullptr;
        }
        cereal::BinaryInputArchive archive(is);
        AIMovementGrid* aiMovementGrid = new AIMovementGrid();
        archive(*aiMovementGrid);
        std::cout << "Binary AI walk grid read successful. " << std::endl;
        return aiMovementGrid;
    }
#endif



};


#endif //LIMONENGINE_AIMOVEMENTGRID_H
