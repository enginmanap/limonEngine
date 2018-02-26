//
// Created by engin on 1.01.2018.
//

#include "AIMovementGrid.h"

//FIXME: this must be the worst way to check for a node in a graph, when you already implemented a*
AIMovementNode *AIMovementGrid::isAlreadyVisited(const AIMovementNode *node) {
    for (int i = visited.size() - 1; i >= 0; --i) {
        if (isPositionCloseEnough(node->getPosition(), visited[i]->getPosition())) {
            return visited[i];
        }
    }
    return nullptr;
}

const AIMovementNode *
AIMovementGrid::aStarPath(const AIMovementNode *start, const glm::vec3 &destination, std::vector<glm::vec3> *route) {
    std::priority_queue<AINodeWithPriority, std::vector<AINodeWithPriority>, std::greater<AINodeWithPriority>> frontier;
    frontier.push(AINodeWithPriority(start, 0));

    std::map<const AIMovementNode *, const AIMovementNode *> from;
    std::map<const AIMovementNode *, float> totalCost;
    const AIMovementNode *finalNode = nullptr;
    from[start] = nullptr;
    totalCost[start] = 0;
    while (!frontier.empty()) {
        AINodeWithPriority nodeWithPriority = frontier.top();
        //std::cout << "testing with a* " << GLMUtils::vectorToString(nodeWithPriority.node->getPosition()) << std::endl;
        frontier.pop();
        if (isPositionCloseEnough(destination, nodeWithPriority.node->getPosition())) {
            finalNode = nodeWithPriority.node;
            break;
        }

        for (int i = 0; i < 9; ++i) {
            AIMovementNode *currentNode = nodeWithPriority.node->getNeighbour(i);
            if (currentNode == nullptr) {
                continue;
            }
            if (!currentNode->isIsMovable()) {
                continue;//if not movable, it means we don't need its child
            }
            //make going cross a little harder
            float movementCost = glm::length(currentNode->getPosition() - nodeWithPriority.node->getPosition());
            float heuristic = glm::length(destination - currentNode->getPosition());
            float currentCost = totalCost[nodeWithPriority.node] + movementCost + heuristic;
            if (!totalCost.count(currentNode) || currentCost < totalCost[currentNode]) {
                //float currentPriority = currentCost + heuristic;
//                std::cout << "for node " << GLMUtils::vectorToString(currentNode->getPosition()) << " priority is " << currentCost << " with heuristic " << heuristic << std::endl;
                frontier.push(AINodeWithPriority(currentNode, currentCost));
                from[currentNode] = nodeWithPriority.node;
                totalCost[currentNode] = currentCost;
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
        const AIMovementNode *fromNode = from[finalNode];
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
 * @param position
 * @param floatingHeight
 * @param checkHeight
 * @param staticWorld
 * @return
 */
bool AIMovementGrid::setProperHeight(glm::vec3 *position, float floatingHeight, float checkHeight,
                                     btDiscreteDynamicsWorld *staticWorld) {
    rayCallback->m_rayFromWorld = GLMConverter::GLMToBlt(*position);
    if (checkHeight < 0.01) {
        rayCallback->m_rayToWorld = GLMConverter::GLMToBlt(
                *position - glm::vec3(0, 9999999, 0));//FIXME this number is for testing only
    } else {
        rayCallback->m_rayToWorld = GLMConverter::GLMToBlt(*position - glm::vec3(0, checkHeight, 0));
    }
    rayCallback->m_closestHitFraction = 1;
    rayCallback->m_collisionObject = nullptr;
    staticWorld->rayTest(rayCallback->m_rayFromWorld, rayCallback->m_rayToWorld, *rayCallback);
    if (rayCallback->hasHit()) {
        //std::cout << "the object that hit is " << *((std::string*)rayCallback->m_collisionObject->getUserPointer()) << std::endl;
        position->y = rayCallback->m_hitPointWorld.getY() + floatingHeight;
        return true;
    } else {
        return false;
    }
}

AIMovementNode *
AIMovementGrid::walkMonster(glm::vec3 walkPoint, btDiscreteDynamicsWorld *staticWorld, const glm::vec3 &min,
                            const glm::vec3 &max) {
    std::queue<AIMovementNode *> frontier;
    if (!setProperHeight(&walkPoint, floatingHeight, 0.0f, staticWorld)) {
        std::cerr << "Root node has nothing underneath, grid generation failed. " << std::endl;
        return root;
    }
    AIMovementNode *root = new AIMovementNode(walkPoint);
    staticWorld->addCollisionObject(sharedGhostObject, btBroadphaseProxy::SensorTrigger,
                                    btBroadphaseProxy::AllFilter & ~btBroadphaseProxy::SensorTrigger);
    sharedGhostObject->setWorldTransform(
            btTransform(btQuaternion::getIdentity(), GLMConverter::GLMToBlt(root->getPosition())));
    bool isMovable = !isThereCollision(staticWorld);
    staticWorld->removeCollisionObject(sharedGhostObject);
    if (isMovable) {
        root->setIsMovable(isMovable);
        frontier.push(root);
        visited.push_back(root);
    } else {
        std::cerr << "Root node " << GLMUtils::vectorToString(walkPoint) << "is not movable, AI walk grid generation failed. Please check map." << std::endl;
        return root;
    }
    AIMovementNode *current;
    while (!frontier.empty()) {
        current = frontier.front();
        frontier.pop();
        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                if (i == 0 && j == 0) {
                    continue; //skip the center, it is the self
                } else {
                    if (current->getPosition().x < 1.1 && current->getPosition().x > -1.1 &&
                        current->getPosition().z < 0.1 && current->getPosition().z > -0.1) {
                        std::cout << "setting neigbours for 0,0 and y " << current->getPosition().y << std::endl;
                    }
                    int neighbourIndex = (i + 1) * 3 + (j + 1);
                    glm::vec3 neighbourPosition = current->getPosition() + glm::vec3(i, 0, j);
                    if (!setProperHeight(&neighbourPosition, floatingHeight, floatingHeight + 1.0f, staticWorld)) {
                        current->setNeighbour(neighbourIndex, nullptr);
                        continue;// if there is nothing under for 1.5f, than don't process this node.
                    }

                    if (neighbourPosition.x < min.x || neighbourPosition.y < min.y || neighbourPosition.z < min.z ||
                        neighbourPosition.x > max.x || neighbourPosition.y > max.y || neighbourPosition.z > max.z) {
                        //this means this position is out of whole world AABB, skip
                        continue;
                    }
                    AIMovementNode *neighbour = new AIMovementNode(neighbourPosition);
                    AIMovementNode *visitedNode = isAlreadyVisited(neighbour);
                    if (visitedNode != nullptr) {
                        //std::cout << "already visited node at " << GLMUtils::vectorToString(neighbourPosition) << std::endl;
                        //std::cout << "already visited node position is " << GLMUtils::vectorToString(visitedNode->getPosition()) << std::endl;
                        current->setNeighbour(neighbourIndex, visitedNode);
                        delete neighbour;
                    } else {
//                        std::cout << "adding new node with position " << GLMUtils::vectorToString(neighbourPosition) << std::endl;
                        staticWorld->addCollisionObject(sharedGhostObject, btBroadphaseProxy::SensorTrigger,
                                                        btBroadphaseProxy::AllFilter &
                                                        ~btBroadphaseProxy::SensorTrigger);
                        sharedGhostObject->setWorldTransform(
                                btTransform(btQuaternion::getIdentity(), GLMConverter::GLMToBlt(neighbourPosition)));
                        isMovable = !isThereCollision(staticWorld);
                        staticWorld->removeCollisionObject(sharedGhostObject);
                        neighbour->setIsMovable(isMovable);
                        current->setNeighbour(neighbourIndex, neighbour);
                        frontier.push(neighbour);
                        visited.push_back(neighbour);

                    }
                }
            }
        }

    }
    return root;
}

bool AIMovementGrid::isThereCollision(btDiscreteDynamicsWorld *staticWorld) {
    isThereCollisionCounter++;
    staticWorld->updateAabbs();//this should not be needed, but it is. I have no idea why
    staticWorld->getDispatcher()->dispatchAllCollisionPairs(sharedGhostObject->getOverlappingPairCache(),
                                                            staticWorld->getDispatchInfo(),
                                                            staticWorld->getDispatcher());
    btBroadphasePairArray &pairArray = sharedGhostObject->getOverlappingPairCache()->getOverlappingPairArray();
    int numPairs = pairArray.size();

    for (int i = 0; i < numPairs; ++i) {
        const btBroadphasePair &pair = pairArray[i];

        if (pair.m_algorithm) {
            pair.m_algorithm->getAllContactManifolds(sharedManifoldArray);
        }

        for (int j = 0; j < sharedManifoldArray.size(); j++) {
            btPersistentManifold *manifold = sharedManifoldArray[j];

            //bool isFirstBody = manifold->getBody0() == sharedGhostObject;

            //btScalar direction = isFirstBody ? btScalar(-1.0) : btScalar(1.0);

            for (int p = 0; p < manifold->getNumContacts(); ++p) {
                const btManifoldPoint &pt = manifold->getContactPoint(p);

                if (pt.getDistance() < 0.f) {
//                    if (true) {
                    // handle collisions here
                    if (GLMConverter::BltToGLM(sharedGhostObject->getWorldTransform().getOrigin()).z < 27 &&
                        GLMConverter::BltToGLM(sharedGhostObject->getWorldTransform().getOrigin()).z > 15 &&
                        GLMConverter::BltToGLM(sharedGhostObject->getWorldTransform().getOrigin()).x < 26 &&
                        GLMConverter::BltToGLM(sharedGhostObject->getWorldTransform().getOrigin()).x > -4) {
                        std::cout << "ghost object collision at " << GLMUtils::vectorToString(
                                GLMConverter::BltToGLM(sharedGhostObject->getWorldTransform().getOrigin()))
                                  << std::endl;
                        std::cout << "collided with" << *(std::string *) (manifold->getBody1()->getUserPointer())
                                  << std::endl;
//                            const btVector3& ptA = pt.getPositionWorldOnA();
//                            const btVector3& ptB = pt.getPositionWorldOnB();
//                            const btVector3& normalOnB = pt.m_normalWorldOnB;


                    }
                    return true;
                    //debugDrawer->drawLine(GLMConverter::BltToGLM(ptA), GLMConverter::BltToGLM(ptA) + )
                }
            }
        }
        sharedManifoldArray.resize(0);

    }
    return false;
}

AIMovementGrid::AIMovementGrid(glm::vec3 startPoint, btDiscreteDynamicsWorld *staticOnlyPhysicsWorld, glm::vec3 min,
                               glm::vec3 max) {
    //sharedGhostObject->setCollisionShape(new btBoxShape(btVector3(1.0f,1.0f,1.0f)));
    //sharedGhostObject->setCollisionShape(new btCapsuleShape(1,1));
    ghostShape = new btCapsuleShape(capsuleRadius, capsuleHeight);
    sharedGhostObject->setCollisionShape(ghostShape);
    std::string ghostSTR = "GHOST_OBJECT";
    sharedGhostObject->setUserPointer(&ghostSTR);
    sharedGhostObject->setCollisionFlags(
            sharedGhostObject->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE);
    sharedGhostObject->setWorldTransform(btTransform(btQuaternion::getIdentity(), GLMConverter::GLMToBlt(startPoint)));
    std::cout << "Start generating AI walk grid" << std::endl;
    root = walkMonster(startPoint, staticOnlyPhysicsWorld, min, max);
    std::cout << "Finished generating AI walk grid, created " << visited.size() << " nodes, checked for collision "
              << isThereCollisionCounter << " times." << std::endl;
    staticOnlyPhysicsWorld->removeCollisionObject(sharedGhostObject);
}

bool
AIMovementGrid::coursePath(const glm::vec3 &from, const glm::vec3 &to, int actorId, std::vector<glm::vec3> *route) {

    //first search for from node.
    const AIMovementNode *fromAINode = nullptr;
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
    fromAINode = aStarPath(fromAINode, from, route);

    if (fromAINode == nullptr) {
        std::cerr << "new from node can't be found, this means snap distance is too small." << std::endl;
        return false;
    }

    //save actor position to use on later calls
    actorLastNodeMap[actorId] = fromAINode;

    const AIMovementNode *finalNode = aStarPath(fromAINode, to, route);

    if (finalNode == nullptr) {
        std::cerr << "Destination can't be reached, most likely player moved to somewhere AI can't." << std::endl;
        return false;
    } else {
        return true;
    }
}

bool AIMovementGrid::coursePath(const glm::vec3 &from, const glm::vec3 &to, std::vector<glm::vec3> *route) {
    //first start by finding the from point. We should  cache these from values at some point, so we don't a* twice all the time
    const AIMovementNode *fromAINode;

    long start = SDL_GetTicks();

    fromAINode = aStarPath(root, from, route);
    if (fromAINode == nullptr) {
        return false;
    }

    aStarPath(fromAINode, to, route);
    long end = SDL_GetTicks();
    std::cout << "route set " << end - start << std::endl;



    return true;
}

void AIMovementGrid::debugDraw(BulletDebugDrawer *debugDrawer) const {
    std::vector<AIMovementNode *>::const_iterator it;
    glm::vec3 toColor, fromColor;
    for (it = visited.begin(); it != visited.end(); it++) {
        int neighbourCount = 5; //if not an edge, just draw first 4, the last 4 should be rendered by the neighbours
        if ((*it)->isIsMovable()) {
            fromColor = glm::vec3(1, 1, 1);
        } else {
            fromColor = glm::vec3(1, 0, 0);
            neighbourCount = 9; //if on an edge, neighbours will not be able to draw rest, draw all.
        }
        for (int j = 0; j < neighbourCount; ++j) {
            if ((*it)->getNeighbour(j) != nullptr) {
                if ((*it)->getNeighbour(j)->isIsMovable()) {
                    toColor = glm::vec3(1, 1, 1);
                } else {
                    toColor = glm::vec3(1, 0, 0);
                }
                debugDrawer->drawLine(GLMConverter::GLMToBlt((*it)->getPosition()),
                                      GLMConverter::GLMToBlt((*it)->getNeighbour(j)->getPosition()),
                                      GLMConverter::GLMToBlt(fromColor), GLMConverter::GLMToBlt(toColor));
            }
        }

    }
}
