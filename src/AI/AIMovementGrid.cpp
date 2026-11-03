//
// Created by engin on 1.01.2018.
//

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <glm/ext.hpp>
#include "AIMovementGrid.h"

#include "SDL3Helper.h"
#include "../LimonFileSystem.h"
#include "../GameObjects/GameObject.h"
#include "../limonAPI/util/HashUtil.h"

void AIMovementGrid::addToVisited(const std::shared_ptr<AIMovementNode> &node) {
    visited[getCellKey(node->getPosition())].push_back(node);
    visitedCount++;
}

void AIMovementGrid::removeFromVisited(const std::shared_ptr<AIMovementNode> &node) {
    std::unordered_map<uint64_t, std::vector<std::shared_ptr<AIMovementNode>>>::iterator cellIt = visited.find(getCellKey(node->getPosition()));
    if (cellIt != visited.end()) {
        std::vector<std::shared_ptr<AIMovementNode>> &cell = cellIt->second;
        for (size_t i = 0; i < cell.size(); ++i) {
            if (cell[i] == node) {
                cell.erase(cell.begin() + i);
                visitedCount--;
                if (cell.empty()) {
                    visited.erase(cellIt);
                }
                return;
            }
        }
    }
    std::cerr << "AI grid finished a node that is not in visited, " << glm::to_string(node->getPosition()) << std::endl;
    std::exit(-1);
}

void AIMovementGrid::finishNode(const std::shared_ptr<AIMovementNode> &node) {
    removeFromVisited(node);
    doneNodes.push_back(node);
    doneCells[getCellKey(node->getPosition())].push_back(node);
    this->max = glm::max(this->max, node->getPosition());
    this->min = glm::min(this->min, node->getPosition());
}

const std::shared_ptr<AIMovementNode> *AIMovementGrid::findNode(const glm::vec3 &position) const {
    uint64_t cellKey = getCellKey(position);
    const std::unordered_map<uint64_t, std::vector<std::shared_ptr<AIMovementNode>>> *tables[2] = {&visited, &doneCells};
    for (const std::unordered_map<uint64_t, std::vector<std::shared_ptr<AIMovementNode>>> *table : tables) {
        std::unordered_map<uint64_t, std::vector<std::shared_ptr<AIMovementNode>>>::const_iterator cellIt = table->find(cellKey);
        if (cellIt == table->end()) {
            continue;
        }
        for (const std::shared_ptr<AIMovementNode> &candidate : cellIt->second) {
            if (isPositionCloseEnoughYOnly(position, candidate->getPosition())) {
                return &candidate;
            }
        }
    }
    return nullptr;
}

bool AIMovementGrid::isPositionKnown(const glm::vec3 &position) const {
    return findNode(position) != nullptr;
}

bool AIMovementGrid::isDiagonalMoveAllowed(const std::shared_ptr<const AIMovementNode> &node, int neighbourIndex) {
    //index is (x offset + 1) * 3 + (z offset + 1). A diagonal one is blocked by either orthogonal neighbour that shares an axis with it, a wall corner between two boxes would otherwise be squeezed through
    int firstOrthogonal;
    int secondOrthogonal;
    switch (neighbourIndex) {
        case 0: firstOrthogonal = 1; secondOrthogonal = 3; break;
        case 2: firstOrthogonal = 1; secondOrthogonal = 5; break;
        case 6: firstOrthogonal = 7; secondOrthogonal = 3; break;
        case 8: firstOrthogonal = 7; secondOrthogonal = 5; break;
        default: return true;
    }
    std::shared_ptr<AIMovementNode> first = node->getNeighbour(firstOrthogonal);
    std::shared_ptr<AIMovementNode> second = node->getNeighbour(secondOrthogonal);
    return first != nullptr && first->isIsMovable() && second != nullptr && second->isIsMovable();
}

std::shared_ptr<const AIMovementNode>
AIMovementGrid::aStarPath(std::shared_ptr<const AIMovementNode> start, const glm::vec3 &destination, uint32_t maximumNumberOfNodes,
                          std::vector<glm::vec3> *route) {
    if (start == nullptr) {
        //no root, grid generation failed. Nothing to search from
        return nullptr;
    }

    std::priority_queue<AINodeWithPriority, std::vector<AINodeWithPriority>, std::greater<AINodeWithPriority>> frontier;
    frontier.push(AINodeWithPriority(start, 0));

    std::map<std::shared_ptr<const AIMovementNode>, std::shared_ptr<const AIMovementNode>> from;
    std::map<std::shared_ptr<const AIMovementNode> , float> totalCost;
    std::map<std::shared_ptr<const AIMovementNode> , uint32_t> totalNodes;
    std::shared_ptr<const AIMovementNode> finalNode = nullptr;
    from[start] = nullptr;
    totalCost[start] = 0;
    totalNodes[start] = 0;
    while (!frontier.empty()) {
        AINodeWithPriority nodeWithPriority = frontier.top();
        //std::cout << "testing with a* " << GLMUtils::vectorToString(nodeWithPriority.node->getPosition()) << std::endl;
        frontier.pop();
        if (isPositionCloseEnough(destination, nodeWithPriority.node->getPosition())) {
            finalNode = nodeWithPriority.node;
            break;
        }

        if(maximumNumberOfNodes != 0 && (totalNodes.count(nodeWithPriority.node) != 0 && totalNodes[nodeWithPriority.node] >= maximumNumberOfNodes)) {
            //we searched for this depth, but couldn't found the player no need to keep searching
            break;
        }

        for (int i = 0; i < 9; ++i) {
            std::shared_ptr<AIMovementNode> currentNode = nodeWithPriority.node->getNeighbour(i);
            if (currentNode == nullptr) {
                continue;
            }
            if (!currentNode->isIsMovable()) {
                continue;//if not movable, it means we don't need its child
            }
            if (!isDiagonalMoveAllowed(nodeWithPriority.node, i)) {
                continue;
            }
            //make going cross a little harder
            float movementCost = glm::length(currentNode->getPosition() - nodeWithPriority.node->getPosition());
            float heuristic = glm::length(destination - currentNode->getPosition());
            float currentCost = totalCost[nodeWithPriority.node] + movementCost + heuristic;
            float currentNodeCount = totalNodes[nodeWithPriority.node] + 1;
            if (!totalCost.count(currentNode) || currentCost < totalCost[currentNode]) {
                //float currentPriority = currentCost + heuristic;
//                std::cout << "for node " << GLMUtils::vectorToString(currentNode->getPosition()) << " priority is " << currentCost << " with heuristic " << heuristic << std::endl;
                frontier.push(AINodeWithPriority(currentNode, currentCost));
                from[currentNode] = nodeWithPriority.node;
                totalCost[currentNode] = currentCost;
                totalNodes[currentNode] = currentNodeCount;
            }
        }

    }

    if (finalNode == nullptr) {
        std::cerr << "Path search failed, please check the values: " << GLMUtils::vectorToString(start->getPosition())
                  << " to " << GLMUtils::vectorToString(destination) << std::endl;
        return finalNode;
    } else {
        if (start == finalNode) {
            return finalNode;//don't put anything to the route;
        }
        route->clear();
        route->push_back(finalNode->getPosition());
        //std::cout << GLMUtils::vectorToString(finalNode->getPosition()) << ", " << (finalNode->isIsMovable()? " yes":"no") << std::endl;
        std::shared_ptr<const AIMovementNode> fromNode = from[finalNode];
        while (start != fromNode) {
            route->push_back(fromNode->getPosition());
            fromNode = from[fromNode];
            //std::cout << GLMUtils::vectorToString(fromNode->getPosition()) << ", " << (fromNode->isIsMovable()? " yes":"no") << std::endl;
        }
        return finalNode;
    }
}

/**
 * Since this Method checks the world, it is perfectly possible ghost object is detected as valid it point. Remove it before calling this method
 *
 * 1) create ray, from position -> to position - 999999 (for 0 check height) or position - check height
 * 2) if there is a hit, move position height to hit+floating height else return false
 *
 * @param position
 * @param checkHeight
 * @param staticWorld
 * @return
 */
bool AIMovementGrid::setProperHeight(glm::vec3 *position, float checkHeight, btDiscreteDynamicsWorld *staticWorld) {
    rayCallback->m_rayFromWorld = GLMConverter::GLMToBlt(*position);
    if (checkHeight == 0.0) {
        rayCallback->m_rayToWorld = GLMConverter::GLMToBlt(
                *position - glm::vec3(0, 9999999, 0));//Normally, we expect world aabb min y here, but it is not passed.
    } else {
        rayCallback->m_rayToWorld = GLMConverter::GLMToBlt(*position - glm::vec3(0, checkHeight, 0));
    }
    rayCallback->m_closestHitFraction = 1;
    rayCallback->m_collisionObject = nullptr;
    staticWorld->rayTest(rayCallback->m_rayFromWorld, rayCallback->m_rayToWorld, *rayCallback);
    if (rayCallback->hasHit()) {
        // We added support for 2 tags. One completely prevents us from detecting the hit, so we don't process it.
        // Other one is a flag for us to mark that point as not walkable. This is a check for that.
        const GameObject *hitObject = static_cast<const GameObject *>(rayCallback->m_collisionObject->getUserPointer());
        if (hitObject != nullptr && hitObject->hasTag(blockerTagHash)) {
            return false;//same rule as the query context, the closest surface decides
        }
        if(rayCallback->m_hitPointWorld.getY() > (*position - glm::vec3(0, floatingHeight - stepHeight, 0)).y) {
            return false;
        }
        position->y = rayCallback->m_hitPointWorld.getY() + floatingHeight;
        return true;
    } else {
        return false;
    }
}

std::shared_ptr<AIMovementNode>
AIMovementGrid::walkMonster(glm::vec3 walkPoint, btDiscreteDynamicsWorld *staticWorld, const glm::vec3 &min,
                            const glm::vec3 &max, uint32_t collisionGroup, uint32_t collisionMask) {
    if (!findGroundAt(&walkPoint, staticWorld, std::fabs(walkPoint.y - min.y))) {
        std::cerr << "Root node " << glm::to_string(walkPoint)<< " has nothing underneath, grid generation failed. " << std::endl;
        return root;
    }

    std::shared_ptr<AIMovementNode>root = std::make_shared<AIMovementNode>(getNextID(), walkPoint);
    if (isBlockedAt(root->getPosition(), staticWorld, collisionGroup, collisionMask)) {
        std::cerr << "Root node " << GLMUtils::vectorToString(walkPoint) << "is not movable, AI walk grid generation failed. Please check map." << std::endl;
        return root;
    }
    root->setIsMovable(true);
    gridOrigin = walkPoint;
    addToVisited(root);
    std::cerr << "Root node " << GLMUtils::vectorToString(walkPoint) << "is movable, AI walk grid generation starts." << std::endl;

    walkLevels(root, staticWorld, min, max, collisionGroup, collisionMask);
    printSearchSummary(staticWorld);
    return root;
}

//ids, positions, movable flags and links of every node, finished or not. Two grids with the same value are the same grid
uint64_t AIMovementGrid::calculateFingerprint() const {
    std::vector<const AIMovementNode *> nodes;
    for (size_t i = 1; i < doneNodes.size(); ++i) {//0 is the placeholder
        nodes.push_back(doneNodes[i].get());
    }
    for (const std::pair<const uint64_t, std::vector<std::shared_ptr<AIMovementNode>>> &cell : visited) {
        for (const std::shared_ptr<AIMovementNode> &visitedNode : cell.second) {
            nodes.push_back(visitedNode.get());
        }
    }
    std::sort(nodes.begin(), nodes.end(), [](const AIMovementNode *left, const AIMovementNode *right) { return left->getID() < right->getID(); });
    uint64_t fingerprint = 14695981039346656037ULL;
    for (const AIMovementNode *node : nodes) {
        uint32_t positionBits[3];
        std::memcpy(positionBits, &node->getPosition(), sizeof(positionBits));
        uint64_t values[14] = {node->getID(), positionBits[0], positionBits[1], positionBits[2], node->isIsMovable() ? 1u : 0u};
        for (int neighbourIndex = 0; neighbourIndex < 9; ++neighbourIndex) {
            values[5 + neighbourIndex] = node->hasNeighbour(neighbourIndex) ? node->getNeighbour(neighbourIndex)->getID() : 0;
        }
        for (uint64_t value : values) {
            fingerprint = (fingerprint ^ value) * 1099511628211ULL;
        }
    }
    return fingerprint;
}

void AIMovementGrid::printSearchSummary(btDiscreteDynamicsWorld *staticWorld) const {
    printGridStatistics(staticWorld);
    std::cout << "Walk grid creation finished with " << doneNodes.size() << " nodes between " << glm::to_string(this->min) << ", " << glm::to_string(this->max) << std::endl;
    std::cout << "Walk grid fingerprint " << std::hex << calculateFingerprint() << std::dec << std::endl;
}

void AIMovementGrid::proposeNode(size_t nodeIndex, LevelWork &work, AIGridQueryContext &context, WorkerCounters &counters) {
    const std::shared_ptr<AIMovementNode> &node = (*work.level)[nodeIndex];
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            int neighbourIndex = (i + 1) * 3 + (j + 1);
            NeighbourProposal &proposal = work.proposals[nodeIndex * 9 + neighbourIndex];
            proposal = NeighbourProposal();
            if (i == 0 && j == 0) {
                continue;
            }
            if (node->hasNeighbour(neighbourIndex)) {
                continue;
            }
            glm::vec3 neighbourPosition = node->getPosition() + glm::vec3(i * spacing, 0, j * spacing);
            counters.groundQueries++;
            bool isGroundFound = context.setProperHeight(work.world, &neighbourPosition, floatingHeight, stepHeight, floatingHeight + stepHeight);
            if (neighbourPosition.x < work.min.x || neighbourPosition.y < work.min.y || neighbourPosition.z < work.min.z ||
                neighbourPosition.x > work.max.x || neighbourPosition.y > work.max.y || neighbourPosition.z > work.max.z) {
                continue;
            }
            proposal.isSkipped = false;
            proposal.position = neighbourPosition;
            proposal.isGroundFound = isGroundFound;
            if (isPositionKnown(neighbourPosition)) {
                proposal.isKnown = true;//the tables only grow, the merge will find it too
            }
        }
    }
}

//workers take chunks of consecutive nodes from a shared counter, a chunk is a small arc of the wave so its rays touch the same geometry
void AIMovementGrid::proposeNeighbours(size_t workerIndex, LevelWork &work) {
    AIGridQueryContext &context = *workerContexts[workerIndex];
    WorkerCounters &counters = work.workerCounters[workerIndex];
    std::chrono::steady_clock::time_point phaseStart = std::chrono::steady_clock::now();
    size_t levelSize = work.level->size();
    while (true) {
        size_t begin = work.nextItem.fetch_add(chunkSize);
        if (begin >= levelSize) {
            break;
        }
        size_t end = std::min(begin + chunkSize, levelSize);
        for (size_t nodeIndex = begin; nodeIndex < end; ++nodeIndex) {
            proposeNode(nodeIndex, work, context, counters);
        }
    }
    counters.levelBusyNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - phaseStart).count();
}

//the collision test only decides the movable flag, so it runs after the merge on the created nodes, once each
void AIMovementGrid::testCollisions(size_t workerIndex, LevelWork &work) {
    AIGridQueryContext &context = *workerContexts[workerIndex];
    WorkerCounters &counters = work.workerCounters[workerIndex];
    std::chrono::steady_clock::time_point phaseStart = std::chrono::steady_clock::now();
    size_t createdCount = work.created.size();
    while (true) {
        size_t begin = work.nextItem.fetch_add(chunkSize);
        if (begin >= createdCount) {
            break;
        }
        size_t end = std::min(begin + chunkSize, createdCount);
        for (size_t createdIndex = begin; createdIndex < end; ++createdIndex) {
            CreatedNode &created = work.created[createdIndex];
            if (created.isGroundFound) {
                counters.collisionQueries++;
                created.isBlocked = context.isBlocked(work.world, created.node->getPosition(), work.collisionGroup, work.collisionMask);
            }
        }
    }
    counters.levelBusyNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - phaseStart).count();
}

void AIMovementGrid::addBusyTime(const LevelWork &work, int64_t &busySumNanoseconds, int64_t &criticalPathNanoseconds) {
    int64_t slowestNanoseconds = 0;
    for (const WorkerCounters &counters : work.workerCounters) {
        busySumNanoseconds += counters.levelBusyNanoseconds;
        slowestNanoseconds = std::max(slowestNanoseconds, counters.levelBusyNanoseconds);
    }
    criticalPathNanoseconds += slowestNanoseconds;
}

void AIMovementGrid::walkLevels(const std::shared_ptr<AIMovementNode> &rootNode, btDiscreteDynamicsWorld *staticWorld, const glm::vec3 &min,
                                const glm::vec3 &max, uint32_t collisionGroup, uint32_t collisionMask) {
    workerContexts.clear();
    for (size_t workerIndex = 0; workerIndex < threadCount; ++workerIndex) {
        workerContexts.push_back(std::make_unique<AIGridQueryContext>(ghostShape, blockerTagHash, collisionGroup, collisionMask));
    }
    AIGridWorkerPool workerPool(threadCount);
    std::cout << "AI grid search runs on " << threadCount << " threads" << std::endl;

    LevelWork work;
    work.world = staticWorld;
    work.min = min;
    work.max = max;
    work.collisionGroup = collisionGroup;
    work.collisionMask = collisionMask;
    work.workerCounters.assign(threadCount, WorkerCounters());
    int64_t busySumNanoseconds = 0;//all workers, all levels
    int64_t criticalPathNanoseconds = 0;//the slowest worker of every level, what the levels cost without wake ups and the merge

    std::vector<std::shared_ptr<AIMovementNode>> currentLevel;
    std::vector<std::shared_ptr<AIMovementNode>> nextLevel;
    currentLevel.push_back(rootNode);
    size_t nextLogCount = 0;
    size_t levelCount = 0;
    std::chrono::nanoseconds parallelPhaseTime = std::chrono::nanoseconds::zero();
    std::chrono::nanoseconds mergePhaseTime = std::chrono::nanoseconds::zero();
    while (!currentLevel.empty()) {
        work.level = &currentLevel;
        work.proposals.resize(currentLevel.size() * 9);
        work.created.clear();
        work.nextItem.store(0);
        std::chrono::steady_clock::time_point parallelStart = std::chrono::steady_clock::now();
        workerPool.run(std::bind(&AIMovementGrid::proposeNeighbours, this, std::placeholders::_1, std::ref(work)));
        std::chrono::steady_clock::time_point mergeStart = std::chrono::steady_clock::now();
        parallelPhaseTime += mergeStart - parallelStart;
        addBusyTime(work, busySumNanoseconds, criticalPathNanoseconds);

        //same order the serial search runs in. Nodes get created here, whether they are movable is not known yet
        nextLevel.clear();
        for (size_t nodeIndex = 0; nodeIndex < currentLevel.size(); ++nodeIndex) {
            const std::shared_ptr<AIMovementNode> &current = currentLevel[nodeIndex];
            if (doneNodes.size() >= nextLogCount) {
                std::cout << "After " << SDL3Helper::getTicks() << " current done " << doneNodes.size() << " nodes, partial " << visitedCount << " level " << levelCount
                          << " width " << currentLevel.size() << " last node: " << glm::to_string(current->getPosition()) << std::endl;
                nextLogCount = doneNodes.size() + 10000;
            }
            for (int i = -1; i <= 1; ++i) {
                for (int j = -1; j <= 1; ++j) {
                    if (i == 0 && j == 0) {
                        continue;
                    }
                    int neighbourIndex = (i + 1) * 3 + (j + 1);
                    const NeighbourProposal &proposal = work.proposals[nodeIndex * 9 + neighbourIndex];
                    if (proposal.isSkipped || current->getNeighbour(neighbourIndex) != nullptr) {
                        continue;
                    }
                    const std::shared_ptr<AIMovementNode> *knownNode = findNode(proposal.position);
                    if (knownNode != nullptr) {
                        current->setNeighbour(neighbourIndex, *knownNode);
                        continue;
                    }
                    std::shared_ptr<AIMovementNode> neighbour = std::make_shared<AIMovementNode>(getNextID(), proposal.position);
                    current->setNeighbour(neighbourIndex, neighbour);
                    addToVisited(neighbour);
                    CreatedNode created;
                    created.node = neighbour;
                    created.isGroundFound = proposal.isGroundFound;
                    work.created.push_back(created);
                }
            }
            finishNode(current);
        }
        std::chrono::steady_clock::time_point collisionStart = std::chrono::steady_clock::now();
        mergePhaseTime += collisionStart - mergeStart;

        work.nextItem.store(0);
        workerPool.run(std::bind(&AIMovementGrid::testCollisions, this, std::placeholders::_1, std::ref(work)));
        std::chrono::steady_clock::time_point frontierStart = std::chrono::steady_clock::now();
        parallelPhaseTime += frontierStart - collisionStart;
        addBusyTime(work, busySumNanoseconds, criticalPathNanoseconds);

        //creation order again, so the next level is in the order the serial queue would have had
        for (const CreatedNode &created : work.created) {
            bool isMovable = created.isGroundFound && !created.isBlocked;
            created.node->setIsMovable(isMovable);
            if (isMovable) {
                nextLevel.push_back(created.node);
            }
        }
        currentLevel.swap(nextLevel);
        mergePhaseTime += std::chrono::steady_clock::now() - frontierStart;
        levelCount++;
    }
    std::cout << "AI grid search finished after " << levelCount << " levels, parallel queries took "
              << std::chrono::duration_cast<std::chrono::milliseconds>(parallelPhaseTime).count() << " ms, serial merge took "
              << std::chrono::duration_cast<std::chrono::milliseconds>(mergePhaseTime).count() << " ms" << std::endl;
    for (const WorkerCounters &counters : work.workerCounters) {
        groundQueryCount += counters.groundQueries;
        collisionQueryCount += counters.collisionQueries;
    }
    std::cout << "AI grid search workers were busy " << busySumNanoseconds / 1000000 << " ms in total, the slowest worker of every level added up to "
              << criticalPathNanoseconds / 1000000 << " ms, so the levels waited " << (criticalPathNanoseconds * static_cast<int64_t>(threadCount) * 100 / std::max<int64_t>(busySumNanoseconds, 1)) - 100
              << "% above a perfect split, and the wake ups and joins cost " << std::chrono::duration_cast<std::chrono::milliseconds>(parallelPhaseTime).count() - criticalPathNanoseconds / 1000000 << " ms" << std::endl;
}

void AIMovementGrid::printCellDetails(uint64_t cellKey, btDiscreteDynamicsWorld *staticWorld) const {
    float cellX = gridOrigin.x + static_cast<int32_t>(cellKey >> 32);
    float cellZ = gridOrigin.z + static_cast<int32_t>(cellKey & 0xFFFFFFFF);
    std::cout << "  cell x/z " << cellX << ", " << cellZ << " node y values:";
    for (size_t i = 1; i < doneNodes.size(); ++i) {
        if (getCellKey(doneNodes[i]->getPosition()) == cellKey) {
            std::cout << " " << doneNodes[i]->getPosition().y;
        }
    }
    std::cout << std::endl;

    btVector3 rayFrom(cellX, this->max.y + 10.0f, cellZ);
    btVector3 rayTo(cellX, this->min.y - 10.0f, cellZ);
    btCollisionWorld::AllHitsRayResultCallback allHits(rayFrom, rayTo);
    staticWorld->rayTest(rayFrom, rayTo, allHits);
    std::map<float, std::string, std::greater<float>> hitsByHeight;//top to bottom
    for (int i = 0; i < allHits.m_collisionObjects.size(); ++i) {
        std::string hitName = "no game object";
        const GameObject *hitObject = static_cast<const GameObject *>(allHits.m_collisionObjects[i]->getUserPointer());
        if (hitObject != nullptr) {
            hitName = hitObject->getName();
        }
        hitsByHeight[allHits.m_hitPointWorld[i].getY()] = hitName;
    }
    for (const std::pair<const float, std::string> &hit : hitsByHeight) {
        std::cout << "    ray hit y " << hit.first << " " << hit.second << std::endl;
    }
}

void AIMovementGrid::printGridStatistics(btDiscreteDynamicsWorld *staticWorld) const {
    std::unordered_map<uint64_t, uint32_t> nodesPerCell;
    for (size_t i = 1; i < doneNodes.size(); ++i) {//0 is the placeholder
        nodesPerCell[getCellKey(doneNodes[i]->getPosition())]++;
    }
    std::map<uint32_t, uint32_t> cellsByNodeCount;
    std::map<uint32_t, uint64_t> sampleCellByNodeCount;
    uint64_t worstCell = 0;
    uint32_t worstCount = 0;
    for (const std::pair<const uint64_t, uint32_t> &cell : nodesPerCell) {
        cellsByNodeCount[cell.second]++;
        sampleCellByNodeCount.emplace(cell.second, cell.first);//first one seen stays
        if (cell.second > worstCount) {
            worstCount = cell.second;
            worstCell = cell.first;
        }
    }
    std::cout << "Walk grid statistics: " << nodesPerCell.size() << " x/z cells hold " << doneNodes.size() - 1 << " done nodes, " << visitedCount << " nodes left in visited" << std::endl;
    for (const std::pair<const uint32_t, uint32_t> &bucket : cellsByNodeCount) {
        std::cout << "  " << bucket.second << " cells with " << bucket.first << " nodes" << std::endl;
    }
    for (const std::pair<const uint32_t, uint64_t> &sample : sampleCellByNodeCount) {
        std::cout << " sample of a cell with " << sample.first << " nodes:" << std::endl;
        printCellDetails(sample.second, staticWorld);
    }
    std::cout << " cell with most nodes:" << std::endl;
    printCellDetails(worstCell, staticWorld);
}

bool AIMovementGrid::isBlockedAt(const glm::vec3 &position, btDiscreteDynamicsWorld *staticWorld, uint32_t collisionGroup, uint32_t collisionMask) {
    collisionQueryCount++;
    return queryContext->isBlocked(staticWorld, position, collisionGroup, collisionMask);
}

bool AIMovementGrid::findGroundAt(glm::vec3 *position, btDiscreteDynamicsWorld *staticWorld, float checkHeight) {
    groundQueryCount++;
    return queryContext->setProperHeight(staticWorld, position, floatingHeight, stepHeight, checkHeight);
}

bool AIMovementGrid::chooseStartPoint(const std::vector<StartCandidate> &startCandidates, btDiscreteDynamicsWorld *staticWorld, const glm::vec3 &min,
                                      uint32_t collisionGroup, uint32_t collisionMask, glm::vec3 *startPoint) {
    for (const StartCandidate &candidate : startCandidates) {
        glm::vec3 snappedPosition = candidate.position;
        if (!findGroundAt(&snappedPosition, staticWorld, std::fabs(snappedPosition.y - min.y))) {
            std::cout << "AI grid start candidate rejected, " << candidate.description << " at " << glm::to_string(candidate.position) << ": no walkable ground below it" << std::endl;
            continue;
        }
        if (isBlockedAt(snappedPosition, staticWorld, collisionGroup, collisionMask)) {
            std::cout << "AI grid start candidate rejected, " << candidate.description << " at " << glm::to_string(candidate.position) << ": ground found at " << glm::to_string(snappedPosition) << " but the probe collides there" << std::endl;
            continue;
        }
        std::cout << "AI grid start point is " << candidate.description << " at " << glm::to_string(candidate.position) << ", ground at " << glm::to_string(snappedPosition) << std::endl;
        *startPoint = candidate.position;
        return true;
    }
    return false;
}

//the probe has to cover a cell or thin walls slip between the checks, and its bottom has to clear the ground it stands on
bool AIMovementGrid::readOptions(OptionsUtil::Options *options) {
    spacing = static_cast<float>(options->getOption<double>(HASH("ai_gridSpacing")).getOrDefault(1.0));
    floatingHeight = static_cast<float>(options->getOption<double>(HASH("ai_gridFloatingHeight")).getOrDefault(2.0));
    stepHeight = static_cast<float>(options->getOption<double>(HASH("ai_gridStepHeight")).getOrDefault(1.0));
    if (!(spacing > 0.0f) || !(floatingHeight > 0.0f) || !(stepHeight > 0.0f)) {
        std::cerr << "AI grid spacing, floating height and step height must be positive, they are " << spacing << ", " << floatingHeight << ", "
                  << stepHeight << std::endl;
        return false;
    }

    float probeWidth;
    float probeHalfHeight;
    if (options->getOption<std::string>(HASH("ai_gridProbeShape")).getOrDefault(std::string("box")) == "capsule") {
        float radius = static_cast<float>(options->getOption<double>(HASH("ai_gridCapsuleRadius")).getOrDefault(0.71));
        float height = static_cast<float>(options->getOption<double>(HASH("ai_gridCapsuleHeight")).getOrDefault(1.4));
        if (!(radius > 0.0f) || !(height > 0.0f)) {
            std::cerr << "AI grid capsule radius and height must be positive, they are " << radius << ", " << height << std::endl;
            return false;
        }
        probeWidth = 2.0f * radius;
        probeHalfHeight = height / 2.0f + radius;
        ghostShape = new btCapsuleShape(radius, height);
        std::cout << "AI grid probe shape is capsule, radius " << radius << " height " << height << std::endl;
    } else {
        //not round on purpose, a surface exactly on a box face is a coin flip at +-1500 from origin, where floats are 1e-4 apart
        float halfExtentXZ = static_cast<float>(options->getOption<double>(HASH("ai_gridBoxHalfExtentXZ")).getOrDefault(0.5137));
        float halfHeight = static_cast<float>(options->getOption<double>(HASH("ai_gridBoxHalfHeight")).getOrDefault(1.0513));
        if (!(halfExtentXZ > 0.0f) || !(halfHeight > 0.0f)) {
            std::cerr << "AI grid box half extents must be positive, they are " << halfExtentXZ << ", " << halfHeight << std::endl;
            return false;
        }
        probeWidth = 2.0f * halfExtentXZ;
        probeHalfHeight = halfHeight;
        ghostShape = new btBoxShape(btVector3(halfExtentXZ, halfHeight, halfExtentXZ));
        std::cout << "AI grid probe shape is box, half extents " << halfExtentXZ << " " << halfHeight << " " << halfExtentXZ << std::endl;
    }

    if (probeWidth < spacing) {
        std::cerr << "AI grid probe is " << probeWidth << " wide, narrower than the spacing " << spacing << ", thin walls would slip between the nodes" << std::endl;
        return false;
    }
    if (floatingHeight - probeHalfHeight <= 0.0f) {
        std::cerr << "AI grid probe reaches " << probeHalfHeight << " below its node, the floating height " << floatingHeight << " leaves it inside the ground" << std::endl;
        return false;
    }
    std::cout << "AI grid spacing " << spacing << ", floating height " << floatingHeight << ", step height " << stepHeight << std::endl;
    return true;
}

AIMovementGrid::AIMovementGrid(const std::vector<StartCandidate> &startCandidates, btDiscreteDynamicsWorld *staticOnlyPhysicsWorld, glm::vec3 min,
                               glm::vec3 max, uint32_t collisionGroup, uint32_t collisionMask, OptionsUtil::Options *options) {
    long requestedThreads = options->getOption<long>(HASH("ai_gridThreads")).getOrDefault(1L);
    if (requestedThreads == -1) {
        requestedThreads = static_cast<long>(std::thread::hardware_concurrency());//0 when the system can't tell, that ends up as 1
    }
    threadCount = static_cast<size_t>(std::max(1L, requestedThreads));
    chunkSize = static_cast<size_t>(std::max(1L, options->getOption<long>(HASH("ai_gridChunkSize")).getOrDefault(4L)));
    if (threadCount == 1) {
        chunkSize = std::numeric_limits<size_t>::max() / 2;//one worker takes the whole level at once
    }

    if (!readOptions(options)) {
        std::cerr << "AI grid generation refused, fix the ai_grid settings" << std::endl;
        return;
    }

    queryContext = std::make_unique<AIGridQueryContext>(ghostShape, blockerTagHash, collisionGroup, collisionMask);
    if (std::fabs(min.x) > MAX_GRID_CELLS * spacing || std::fabs(min.z) > MAX_GRID_CELLS * spacing ||
        std::fabs(max.x) > MAX_GRID_CELLS * spacing || std::fabs(max.z) > MAX_GRID_CELLS * spacing) {
        std::cerr << "AI walk grid bounds are too large, coordinates must stay within +-" << MAX_GRID_CELLS * spacing << std::endl;
        std::exit(-1);
    }
    std::cout << "Start generating AI walk grid" << std::endl;
    doneNodes.push_back(std::make_shared<AIMovementNode>(0, glm::vec3(0,200,0)));//0 index element should be empty

    glm::vec3 startPoint(0);
    if (!chooseStartPoint(startCandidates, staticOnlyPhysicsWorld, min, collisionGroup, collisionMask, &startPoint)) {
        std::cerr << "AI grid generation failed: none of the " << startCandidates.size() << " start candidates is valid" << std::endl;
        return;
    }

    Uint64 generationStartTicks = SDL3Helper::getTicks();
    root = walkMonster(startPoint, staticOnlyPhysicsWorld, min, max, collisionGroup, collisionMask);
    std::cout << "Finished generating AI walk grid in " << SDL3Helper::getTicks() - generationStartTicks << " ms, with " << doneNodes.size() - 1 << " nodes and "
              << visitedCount << " unmovable edge nodes" << std::endl;
    std::cout << "AI grid queries that ran: " << groundQueryCount << " ground rays, " << collisionQueryCount << " collision tests" << std::endl;
}

bool
AIMovementGrid::coursePath(const glm::vec3 &from, const glm::vec3 &to, uint32_t actorId, uint32_t maximumNumberOfNodes, std::vector<glm::vec3> *route) {

    //first search for from node.
    std::shared_ptr<const AIMovementNode> fromAINode = nullptr;
    if (actorLastNodeMap.find(actorId) != actorLastNodeMap.end()) {
        //if we already processed this actor before, use the last position of that actor we know
        fromAINode = actorLastNodeMap[actorId];
    } else {
        //if we never processed this actor, use root node for search start.
        fromAINode = root;
    }
    if (fromAINode == nullptr) {
        std::cerr << "Old from node turned out to be NULL. This shouldn't have happened" << std::endl;
        return false;
    }

    //search where the actor is
    fromAINode = aStarPath(fromAINode, from, 0, route);//0 means search whole map

    if (fromAINode == nullptr) {
        std::cerr << "new from node can't be found, this means snap distance is too small." << std::endl;
        return false;
    }

    //save actor position to use on later calls
    actorLastNodeMap[actorId] = fromAINode;

    std::shared_ptr<const AIMovementNode >finalNode = aStarPath(fromAINode, to, maximumNumberOfNodes, route);

    if (finalNode == nullptr) {
        std::cerr << "Destination can't be reached, most likely player moved to somewhere AI can't." << std::endl;
        return false;
    } else {
        return true;
    }
}

bool AIMovementGrid::coursePath(const glm::vec3 &from, const glm::vec3 &to, uint32_t maximumNumberOfNodes, std::vector<glm::vec3> *route) {
    //first start by finding the from point. We should  cache these from values at some point, so we don't a* twice all the time
    std::shared_ptr<const AIMovementNode> fromAINode;

    Uint64 start = SDL3Helper::getTicks();

    fromAINode = aStarPath(root, from, 0, route);
    if (fromAINode == nullptr) {
        return false;
    }

    aStarPath(fromAINode, to, maximumNumberOfNodes, route);
    Uint64 end = SDL3Helper::getTicks();
    std::cout << "route set " << end - start << std::endl;



    return true;
}

void AIMovementGrid::buildDebugCells() const {
    for (size_t i = 1; i < doneNodes.size(); ++i) {//0 is the placeholder
        debugCells[getCellKey(doneNodes[i]->getPosition())].push_back(doneNodes[i].get());
    }
    for (const std::pair<const uint64_t, std::vector<std::shared_ptr<AIMovementNode>>> &cell : visited) {
        for (const std::shared_ptr<AIMovementNode> &visitedNode : cell.second) {
            debugCells[getCellKey(visitedNode->getPosition())].push_back(visitedNode.get());
        }
    }
    isDebugCellsBuilt = true;
}

void AIMovementGrid::drawNodeLines(const AIMovementNode *node, BulletDebugDrawer *debugDrawer) const {
    glm::vec3 fromColor, toColor;
    int neighbourCount = 5; //if not an edge, just draw first 4, the last 4 should be rendered by the neighbours
    if (node->isIsMovable()) {
        fromColor = glm::vec3(1, 1, 1);
    } else {
        fromColor = glm::vec3(1, 0, 0);
        neighbourCount = 9; //if on an edge, neighbours will not be able to draw rest, draw all.
    }
    for (int j = 0; j < neighbourCount; ++j) {
        if (node->getNeighbour(j) != nullptr) {
            if (node->getNeighbour(j)->isIsMovable()) {
                toColor = glm::vec3(1, 1, 1);
            } else {
                toColor = glm::vec3(1, 0, 0);
            }
            debugDrawer->drawLine(GLMConverter::GLMToBlt(node->getPosition()),
                                  GLMConverter::GLMToBlt(node->getNeighbour(j)->getPosition()),
                                  GLMConverter::GLMToBlt(fromColor), GLMConverter::GLMToBlt(toColor));
        }
    }
}

//the cells around the center only
void AIMovementGrid::debugDraw(BulletDebugDrawer *debugDrawer, const glm::vec3 &center, int radius) const {
    if (!isDebugCellsBuilt) {
        buildDebugCells();
    }
    int32_t centerCellX = static_cast<int32_t>(std::round((center.x - gridOrigin.x) / spacing));
    int32_t centerCellZ = static_cast<int32_t>(std::round((center.z - gridOrigin.z) / spacing));
    for (int32_t cellX = centerCellX - radius; cellX <= centerCellX + radius; ++cellX) {
        for (int32_t cellZ = centerCellZ - radius; cellZ <= centerCellZ + radius; ++cellZ) {
            std::unordered_map<uint64_t, std::vector<const AIMovementNode *>>::const_iterator cellIt = debugCells.find(makeCellKey(cellX, cellZ));
            if (cellIt == debugCells.end()) {
                continue;
            }
            for (const AIMovementNode *node : cellIt->second) {
                drawNodeLines(node, debugDrawer);
            }
        }
    }
}

bool AIMovementGrid::serializeXML(const std::string &fileName) {
    tinyxml2::XMLDocument aiGridDocument;
    tinyxml2::XMLNode * rootNode = aiGridDocument.NewElement("AIWalkGrid");
    aiGridDocument.InsertFirstChild(rootNode);

    tinyxml2::XMLElement* currentElement = aiGridDocument.NewElement("Name");
    currentElement->SetText(fileName.c_str());
    rootNode->InsertEndChild(currentElement);

    currentElement = aiGridDocument.NewElement("Version");
    currentElement->SetText(FILE_VERSION);
    rootNode->InsertEndChild(currentElement);

    currentElement = aiGridDocument.NewElement("Spacing");
    currentElement->SetText(spacing);
    rootNode->InsertEndChild(currentElement);

    currentElement = aiGridDocument.NewElement("StepHeight");
    currentElement->SetText(stepHeight);
    rootNode->InsertEndChild(currentElement);

    currentElement = aiGridDocument.NewElement("FloatingHeight");
    currentElement->SetText(floatingHeight);
    rootNode->InsertEndChild(currentElement);

    currentElement = aiGridDocument.NewElement("MaximumNodeID");
    currentElement->SetText(nextPossibleIndex);
    rootNode->InsertEndChild(currentElement);

    tinyxml2::XMLElement * nodesElement = aiGridDocument.NewElement("Nodes");

    for (auto nodeIt = doneNodes.begin(); nodeIt != doneNodes.end(); ++nodeIt) {
        serializeNode(aiGridDocument, rootNode, *nodeIt);
    }

    for (const std::pair<const uint64_t, std::vector<std::shared_ptr<AIMovementNode>>> &cell : visited) {
        for (const std::shared_ptr<AIMovementNode> &visitedNode : cell.second) {
            serializeNode(aiGridDocument, rootNode, visitedNode);
        }
    }

    rootNode->InsertEndChild(nodesElement);


    tinyxml2::XMLError eResult = aiGridDocument.SaveFile(fileName.c_str());
    if(eResult != tinyxml2::XML_SUCCESS) {
        std::cerr  << "ERROR saving AI grid: " << eResult << std::endl;
        return false;
    }
    return true;
}

void AIMovementGrid::serializeNode(tinyxml2::XMLDocument &aiGridDocument, tinyxml2::XMLNode *rootNode, std::shared_ptr<const AIMovementNode> nodeToSerialize) const {
    tinyxml2::XMLElement* nodeElement = aiGridDocument.NewElement("Node");

    tinyxml2::XMLElement *currentElement = aiGridDocument.NewElement("ID");
    currentElement->SetText(std::to_string(nodeToSerialize->getID()).c_str());
    nodeElement->InsertEndChild(currentElement);

    currentElement = aiGridDocument.NewElement("Mv");
    currentElement->SetText((nodeToSerialize->isIsMovable() ? "True" : "False"));
    nodeElement->InsertEndChild(currentElement);

    currentElement = aiGridDocument.NewElement("Ps");
    {
            tinyxml2::XMLElement *positionXElement = aiGridDocument.NewElement("X");
            positionXElement->SetText(std::to_string(nodeToSerialize->getPosition().x).c_str());
            currentElement->InsertEndChild(positionXElement);

            tinyxml2::XMLElement *positionYElement = aiGridDocument.NewElement("Y");
            positionYElement->SetText(std::to_string(nodeToSerialize->getPosition().y).c_str());
            currentElement->InsertEndChild(positionYElement);

            tinyxml2::XMLElement *positionZElement = aiGridDocument.NewElement("Z");
            positionZElement->SetText(std::to_string(nodeToSerialize->getPosition().z).c_str());
            currentElement->InsertEndChild(positionZElement);

        }
    nodeElement->InsertEndChild(currentElement);


    for (int i = 0; i < 9; ++i) {
            if(i == 4) {
                continue;//4 is self
            }
            currentElement = aiGridDocument.NewElement("Nb");//neighbour
            currentElement->SetAttribute("Ps", std::to_string(i).c_str());//position
            if(nodeToSerialize->getNeighbour(i) != nullptr) {
                currentElement->SetText(std::to_string(nodeToSerialize->getNeighbour(i)->getID()).c_str());
            } else {
                currentElement->SetText("0");//writing 0 if null
            }
            nodeElement->InsertEndChild(currentElement);
        }
    rootNode->InsertEndChild(nodeElement);
}

AIMovementGrid *AIMovementGrid::deserialize(const std::string &fileName) {
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = LimonFileSystem::loadXML(fileName, xmlDoc);
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Error loading XML "<< fileName << ": " <<  tinyxml2::XMLDocument::ErrorIDToName(eResult) << std::endl;
        std::cerr << "Possible results: AI actors not moving and very slow map load" << std::endl;
        return nullptr;
    }

    tinyxml2::XMLNode * AIWalkGridRootElement = xmlDoc.FirstChild();
    if (AIWalkGridRootElement == nullptr) {
        std::cerr << fileName << " is not a valid AIWalkGrid file." << std::endl;
        std::cerr << "Possible results: AI actors not moving and very slow map load" << std::endl;
        return nullptr;
    }

    tinyxml2::XMLElement* fileNameNode =  AIWalkGridRootElement->FirstChildElement("Name");
    if (fileNameNode == nullptr) {
        std::cerr << "AIWalkGrid has no fileName in file. Possibly corrupted file." << std::endl;
    }
    else {
        std::cout << "Reading AI WalkGrid from file:" << fileNameNode->GetText() << std::endl;
    }

    tinyxml2::XMLElement *versionElement = AIWalkGridRootElement->FirstChildElement("Version");
    if (versionElement == nullptr || versionElement->GetText() == nullptr || std::strtoul(versionElement->GetText(), nullptr, 0) != FILE_VERSION) {
        std::cerr << fileName << " is in the old AI walk grid format, it has to be generated again" << std::endl;
        return nullptr;
    }
    float fileSpacing = 0;
    float fileStepHeight = 0;
    float fileFloatingHeight = 0;
    tinyxml2::XMLElement *spacingElement = AIWalkGridRootElement->FirstChildElement("Spacing");
    tinyxml2::XMLElement *stepHeightElement = AIWalkGridRootElement->FirstChildElement("StepHeight");
    tinyxml2::XMLElement *floatingHeightElement = AIWalkGridRootElement->FirstChildElement("FloatingHeight");
    if (spacingElement == nullptr || spacingElement->QueryFloatText(&fileSpacing) != tinyxml2::XML_SUCCESS ||
        stepHeightElement == nullptr || stepHeightElement->QueryFloatText(&fileStepHeight) != tinyxml2::XML_SUCCESS ||
        floatingHeightElement == nullptr || floatingHeightElement->QueryFloatText(&fileFloatingHeight) != tinyxml2::XML_SUCCESS ||
        !(fileSpacing > 0.0f) || !(fileStepHeight > 0.0f) || !(fileFloatingHeight > 0.0f)) {
        std::cerr << fileName << " has missing or non positive grid settings, it has to be generated again" << std::endl;
        return nullptr;
    }

    uint32_t maximumNodeID = 0;
    tinyxml2::XMLElement* maximumNodeIDNode =  AIWalkGridRootElement->FirstChildElement("MaximumNodeID");
    if(maximumNodeIDNode != nullptr) {
        maximumNodeID = std::strtoul(maximumNodeIDNode->GetText(), nullptr, 0);
    } else {
        std::cerr << "Maximum number of nodes at AI Walk Grid was missing from file" << fileName << " possible corruption. AI grid load fails." << std::endl;
        std::cerr << "Possible results: AI actors not moving and very slow map load" << std::endl;
        return nullptr;
    }

    AIMovementGrid* grid = new AIMovementGrid();
    grid->spacing = fileSpacing;
    grid->stepHeight = fileStepHeight;
    grid->floatingHeight = fileFloatingHeight;

    for (uint32_t i = 0; i < maximumNodeID; ++i) {
        grid->doneNodes.push_back(std::make_shared<AIMovementNode>(0, glm::vec3(0,100,0)));//we are creating nodes empty, so we can link them together later
    }
    grid->nextPossibleIndex = maximumNodeID;
    tinyxml2::XMLElement* nodeElement =  AIWalkGridRootElement->FirstChildElement("Node");
    while(nodeElement != nullptr) {
        uint32_t nodeID = 0;
        tinyxml2::XMLElement* nodeIDElement =  nodeElement->FirstChildElement("ID");
        if(nodeIDElement != nullptr) {
            nodeID = std::strtoul(nodeIDElement->GetText(), nullptr, 0);
        } else {
            std::cerr << "Missing NodeID at " << fileName << " possible corruption. AI grid load fails." << std::endl;
            std::cerr << "Possible results: AI actors not moving and very slow map load" << std::endl;
            delete grid;
            return nullptr;
        }
        std::shared_ptr<AIMovementNode> thisNode = grid->doneNodes[nodeID];
        thisNode->setID(nodeID);
        if(nodeID == 1) {
                grid->root = thisNode;
        }

        tinyxml2::XMLElement* nodeMovableElement =  nodeElement->FirstChildElement("Mv");//movable
        if(nodeMovableElement != nullptr && nodeMovableElement->GetText() != nullptr) {
            std::string movableString = nodeMovableElement->GetText();
            if(movableString == "True") {
                thisNode->setIsMovable(true);
            } else if(movableString == "False") {
                thisNode->setIsMovable(false);
            } else {
                std::cerr << "Invalid Node movable at " << fileName << " for node " << nodeID << " possible corruption. Assuming False." << std::endl;
                thisNode->setIsMovable(false);
            }
        } else {
            std::cerr << "Missing Node movable at " << fileName << " for node " << nodeID << " possible corruption. Assuming False." << std::endl;
            thisNode->setIsMovable(false);
        }

        /***************** Parse Position *****************/
        tinyxml2::XMLElement* nodePositionElement =  nodeElement->FirstChildElement("Ps");
        if(nodePositionElement != nullptr) {
            tinyxml2::XMLElement* positionXElement =  nodePositionElement->FirstChildElement("X");
            tinyxml2::XMLElement* positionYElement =  nodePositionElement->FirstChildElement("Y");
            tinyxml2::XMLElement* positionZElement =  nodePositionElement->FirstChildElement("Z");

            if(positionXElement == nullptr || positionYElement == nullptr || positionZElement == nullptr ||
               positionXElement->GetText() == nullptr || positionYElement->GetText() == nullptr || positionZElement->GetText() == nullptr) {
                std::cerr << "Missing Node position value at " << fileName << " with ID " << nodeID << " possible corruption. AI grid load fails." << std::endl;
                std::cerr << "Possible results: AI actors not moving and very slow map load" << std::endl;
                delete grid;
                return nullptr;
            } else {
                glm::vec3 position;
                position.x = std::stof(positionXElement->GetText());
                position.y = std::stof(positionYElement->GetText());
                position.z = std::stof(positionZElement->GetText());
                thisNode->setPosition(position);
            }
        } else {
            std::cerr << "Missing Node position at " << fileName << " with ID " << nodeID << " possible corruption. AI grid load fails." << std::endl;
            std::cerr << "Possible results: AI actors not moving and very slow map load" << std::endl;
            delete grid;
            return nullptr;
        }

        /***************** Parse Position *****************/

        tinyxml2::XMLElement* neighbourElement =  nodeElement->FirstChildElement("Nb");
        while(neighbourElement != nullptr) {
            const char* positionAttribute = neighbourElement->Attribute("Ps");
            if(positionAttribute == nullptr) {
                std::cerr << "Missing Node position at " << fileName << " possible corruption. AI grid load fails." << std::endl;
                std::cerr << "Possible results: AI actors not moving and very slow map load" << std::endl;
                delete grid;
                return nullptr;
            } else {
                int neighbourIndex;//this will hold which neighbour was the value
                neighbourIndex = std::stoi(positionAttribute);

                if(neighbourElement->GetText() != nullptr) {
                    uint32_t neighbourPosition = std::strtoul(neighbourElement->GetText(), nullptr, 0);//this is the position in doneNodes array
                    if(neighbourPosition != 0) {
                        thisNode->setNeighbour(neighbourIndex, grid->doneNodes[neighbourPosition]);
                    }//if 0, null
                } else {
                    std::cerr << "Missing Node index at " << fileName << " possible corruption. AI grid load fails." << std::endl;
                    std::cerr << "Possible results: AI actors not moving and very slow map load" << std::endl;
                    delete grid;
                    return nullptr;
                }
            }

            neighbourElement =  neighbourElement->NextSiblingElement("Nb");
        }

        nodeElement =  nodeElement->NextSiblingElement("Node");
    }
    std::cout << "AI Walk grid read successful" << std::endl;
    return grid;
}
