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
#include <unordered_map>
#include <cmath>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>

#ifdef CEREAL_SUPPORT
#include <cereal/access.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include "../Utils/GLMCerealConverters.hpp"
#endif

#include "AIMovementNode.h"
#include "AIGridQueryContext.h"
#include "AIGridWorkerPool.h"
#include "../limonAPI/Options.h"
#include "../Utils/HashUtil.h"
#include "../Utils/HardCodedTags.h"
#include "../LimonFileSystem.h"
#include "../Utils/GLMConverter.h"
#include "../Utils/GLMUtils.h"
#include "../BulletDebugDrawer.h"



//bigger than sqrt(3)/2
//avoiding sqrt
#define GRID_SNAP_DISTANCE (0.8f * 0.8f)

#define X_Z_DISTANCE 0.1
#define Y_DISTANCE_SQ 0.25

class AIMovementGrid {
public:
    struct StartCandidate {
        glm::vec3 position;
        std::string description;//goes to the log, says why this one was in the list
    };

private:
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
    btCollisionWorld::ClosestRayResultCallback *rayCallback = new btCollisionWorld::ClosestRayResultCallback(
            btVector3(0, 0, 0), btVector3(0, 0, 0));
    std::map<int, std::shared_ptr<const AIMovementNode>> actorLastNodeMap;
    uint32_t nextPossibleIndex = 1;//this is to be used internal and constructor only. Not thread safe

    float capsuleHeight = 1.30f + 0.1f;
    float capsuleRadius = 0.71f;//FIXME these should be configurable
    //not round on purpose, a surface exactly on a box face is a coin flip at +-1500 from origin, where floats are 1e-4 apart
    static constexpr float BOX_HALF_EXTENT_XZ = 0.5f + 0.0137f;
    static constexpr float BOX_HALF_HEIGHT = 1.0513f;
    static bool isDiagonalMoveAllowed(const std::shared_ptr<const AIMovementNode> &node, int neighbourIndex);

    std::unique_ptr<AIGridQueryContext> queryContext;
    const uint64_t blockerTagHash = HashUtil::hashString(HardCodedTags::OBJECT_MODEL_AI_WALK_BLOCKER);

    //ai_gridThreads above 1 splits the queries of every level across threads. The merge stays serial and in queue order, so the grid is the same as the serial one
    struct NeighbourProposal {
        glm::vec3 position = glm::vec3(0);
        bool isSkipped = true;//already linked, or out of the world aabb
        bool isGroundFound = false;
        bool isKnown = false;//a node was there before this level started
    };
    struct CreatedNode {
        std::shared_ptr<AIMovementNode> node;
        bool isGroundFound = false;
        bool isBlocked = false;
    };
    struct alignas(64) WorkerCounters {//own cache line, every worker writes only its own
        uint64_t groundQueries = 0;
        uint64_t collisionQueries = 0;
        int64_t levelBusyNanoseconds = 0;//what the worker spent in the phase that just ran
    };
    struct LevelWork {
        std::atomic<size_t> nextItem{0};//next chunk start, shared by the workers of the phase that runs
        std::vector<WorkerCounters> workerCounters;
        std::vector<CreatedNode> created;//what the merge created in this level, in creation order
        const std::vector<std::shared_ptr<AIMovementNode>> *level = nullptr;
        std::vector<NeighbourProposal> proposals;//9 per node of the level, indexed like the neighbours
        btDiscreteDynamicsWorld *world = nullptr;
        glm::vec3 min = glm::vec3(0);
        glm::vec3 max = glm::vec3(0);
        uint32_t collisionGroup = 0;
        uint32_t collisionMask = 0;
    };
    size_t threadCount = 1;
    size_t chunkSize = 4;//nodes a worker takes at a time
    uint64_t groundQueryCount = 0;//what really ran, nodes of one level can't see each other's links yet, so a position can get more than one ray
    uint64_t collisionQueryCount = 0;
    std::vector<std::unique_ptr<AIGridQueryContext>> workerContexts;
    void walkLevels(const std::shared_ptr<AIMovementNode> &rootNode, btDiscreteDynamicsWorld *staticWorld, const glm::vec3 &min,
                    const glm::vec3 &max, uint32_t collisionGroup, uint32_t collisionMask);
    void proposeNode(size_t nodeIndex, LevelWork &work, AIGridQueryContext &context, WorkerCounters &counters);
    void proposeNeighbours(size_t workerIndex, LevelWork &work);
    void testCollisions(size_t workerIndex, LevelWork &work);
    static void addBusyTime(const LevelWork &work, int64_t &busySumNanoseconds, int64_t &criticalPathNanoseconds);
    void finishNode(const std::shared_ptr<AIMovementNode> &node);
    void printSearchSummary(btDiscreteDynamicsWorld *staticWorld) const;
    uint64_t calculateFingerprint() const;
    static bool isLowerId(const AIMovementNode *left, const AIMovementNode *right);
    bool chooseStartPoint(const std::vector<StartCandidate> &startCandidates, btDiscreteDynamicsWorld *staticWorld, const glm::vec3 &min,
                          uint32_t collisionGroup, uint32_t collisionMask, glm::vec3 *startPoint);
    bool isBlockedAt(const glm::vec3 &position, btDiscreteDynamicsWorld *staticWorld, uint32_t collisionGroup, uint32_t collisionMask);
    bool findGroundAt(glm::vec3 *position, btDiscreteDynamicsWorld *staticWorld, float checkHeight);

    glm::vec3 max = glm::vec3(-std::numeric_limits<float>::max());
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    //both keyed by x/z cell, a cell holds one node per floor height
    std::unordered_map<uint64_t, std::vector<std::shared_ptr<AIMovementNode>>> visited;
    size_t visitedCount = 0;
    //finished nodes must stay findable, a miss makes a duplicate that explores everything again
    std::unordered_map<uint64_t, std::vector<std::shared_ptr<AIMovementNode>>> doneCells;
    glm::vec3 gridOrigin = glm::vec3(0);//nodes are whole steps from the root, rounding relative to it keeps them off the cell borders
    std::vector<std::shared_ptr<AIMovementNode>> doneNodes;

    static constexpr float MAX_GRID_COORDINATE = 1.0e9f;//cell coordinates must fit int32_t, even relative to a far away origin

    static uint64_t makeCellKey(int32_t cellX, int32_t cellZ) {
        return (static_cast<uint64_t>(static_cast<uint32_t>(cellX)) << 32) | static_cast<uint32_t>(cellZ);
    }

    uint64_t getCellKey(const glm::vec3 &position) const {
        return makeCellKey(static_cast<int32_t>(std::round(position.x - gridOrigin.x)),
                           static_cast<int32_t>(std::round(position.z - gridOrigin.z)));
    }

    //raw pointers, doneNodes and visited own the nodes
    mutable std::unordered_map<uint64_t, std::vector<const AIMovementNode *>> debugCells;
    mutable bool isDebugCellsBuilt = false;
    void buildDebugCells() const;
    void drawNodeLines(const AIMovementNode *node, BulletDebugDrawer *debugDrawer) const;

    void printGridStatistics(btDiscreteDynamicsWorld *staticWorld) const;
    void printCellDetails(uint64_t cellKey, btDiscreteDynamicsWorld *staticWorld) const;
    void addToVisited(const std::shared_ptr<AIMovementNode> &node);
    void removeFromVisited(const std::shared_ptr<AIMovementNode> &node);
    //the node within the position tolerance, finished or not. Never copies the shared_ptr, a copy is an atomic
    const std::shared_ptr<AIMovementNode> *findNode(const glm::vec3 &position) const;
    std::shared_ptr<AIMovementNode> isAlreadyVisited(const glm::vec3 &position);
    bool isPositionKnown(const glm::vec3 &position) const;

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

    //candidates are tried in order, the first one with ground and room for the probe is the start
    AIMovementGrid(const std::vector<StartCandidate> &startCandidates, btDiscreteDynamicsWorld *staticOnlyPhysicsWorld, glm::vec3 min,
                       glm::vec3 max, uint32_t collisionGroup, uint32_t collisionMask, OptionsUtil::Options *options);

    ~AIMovementGrid() {
        delete rayCallback;
        delete ghostShape;

    }

    bool coursePath(const glm::vec3 &from, const glm::vec3 &to, uint32_t actorId, uint32_t maximumNumberOfNodes,
                    std::vector<glm::vec3> *route);

    bool coursePath(const glm::vec3 &from, const glm::vec3 &to, uint32_t maximumNumberOfNodes, std::vector<glm::vec3> *route);

    //only the cells within radius of the center, the whole grid is too many lines
    void debugDraw(BulletDebugDrawer *debugDrawer, const glm::vec3 &center, int radius) const;

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

        for (const std::pair<const uint64_t, std::vector<std::shared_ptr<AIMovementNode>>> &cell : visited) {
            for (const std::shared_ptr<AIMovementNode> &visitedNode : cell.second) {
                saveNodes[visitedNode->getID()].position = visitedNode->getPosition();
                saveNodes[visitedNode->getID()].movable = visitedNode->isIsMovable();
                for (int j = 0; j < 9; ++j) {
                    if(visitedNode->getNeighbour(j) == nullptr) {
                        saveNodes[visitedNode->getID()].neigbours[j] = 0;
                    } else {
                        saveNodes[visitedNode->getID()].neigbours[j] = visitedNode->getNeighbour(j)->getID();
                    }
                }
                saveNodes[visitedNode->getID()].neigbours[4] = visitedNode->getID();//self ID at 4
            }
        }

        archive(nextPossibleIndex, saveNodes);
    }

    template<class Archive>
    void load(Archive& archive) {
        std::vector<FlattenedNode> saveNodes;
        archive(nextPossibleIndex, saveNodes);

        this->doneNodes.resize(nextPossibleIndex);
        for (uint32_t i = 0; i < nextPossibleIndex; ++i) {
            doneNodes[i] = std::make_shared<AIMovementNode>(i, glm::vec3(0,100,0));//we are creating nodes empty, so we can link them together later
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
        std::vector<uint8_t> fileContents;
        if (!LimonFileSystem::readFile(fileName, fileContents)) {
            std::cout << "Binary AI walk grid read failed. Fallback to XML read" << std::endl;
            return nullptr;
        }
        LimonFileSystem::MemoryStreamBuffer streamBuffer(fileContents);
        std::istream stream(&streamBuffer);
        cereal::BinaryInputArchive archive(stream);
        AIMovementGrid* aiMovementGrid = new AIMovementGrid();
        archive(*aiMovementGrid);
        std::cout << "Binary AI walk grid read successful. " << std::endl;
        return aiMovementGrid;
    }
#endif



};


#endif //LIMONENGINE_AIMOVEMENTGRID_H
