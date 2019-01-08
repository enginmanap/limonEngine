//
// Created by engin on 1.01.2018.
//

#include <glm/ext.hpp>
#include "AIMovementGrid.h"

constexpr float AIMovementGrid::floatingHeight;

//FIXME: this must be the worst way to check for a node in a graph, when you already implemented a*
std::shared_ptr<AIMovementNode>AIMovementGrid::isAlreadyVisited(const glm::vec3 &position, size_t &indexOf) {
    if(visited.empty()) {
        return nullptr;
    }
    for (size_t i = visited.size() - 1; i > 0; --i) {
        if (isPositionCloseEnoughYOnly(position, visited[i]->getPosition())) {
            indexOf = i;
            return visited[i];
        }
    }
    //because size_t wraps around, i>=0 is always true. Instead, = 0 case is below
    if (isPositionCloseEnoughYOnly(position, visited[0]->getPosition())) {
        indexOf = 0;
        return visited[0];
    }
    return nullptr;
}

std::shared_ptr<const AIMovementNode>
AIMovementGrid::aStarPath(std::shared_ptr<const AIMovementNode> start, const glm::vec3 &destination, uint32_t maximumNumberOfNodes,
                          std::vector<glm::vec3> *route) {

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
 * @param floatingHeight
 * @param checkHeight
 * @param staticWorld
 * @return
 */
bool AIMovementGrid::setProperHeight(glm::vec3 *position, float floatingHeight, float checkHeight,
                                     btDiscreteDynamicsWorld *staticWorld) {
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
        if(rayCallback->m_hitPointWorld.getY() > (*position - glm::vec3(0, 1, 0)).y) {
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
    std::queue<std::shared_ptr<AIMovementNode>> frontier;
    if (!setProperHeight(&walkPoint, floatingHeight, std::fabs(walkPoint.y -1 * min.y), staticWorld)) {
        std::cerr << "Root node " << glm::to_string(walkPoint)<< " has nothing underneath, grid generation failed. " << std::endl;
        return root;
    }

    std::shared_ptr<AIMovementNode>root = std::make_shared<AIMovementNode>(getNextID(), walkPoint);
    staticWorld->addCollisionObject(sharedGhostObject, collisionGroup, collisionMask);
    sharedGhostObject->setWorldTransform(
            btTransform(btQuaternion::getIdentity(), GLMConverter::GLMToBlt(root->getPosition())));
    btVector3 minO, maxO;
    sharedGhostObject->getCollisionShape()->getAabb(sharedGhostObject->getWorldTransform(), minO, maxO);
    bool isMovable = !isThereCollision(staticWorld);
    staticWorld->removeCollisionObject(sharedGhostObject);
    if (isMovable) {
        root->setIsMovable(isMovable);
        frontier.push(root);
        visited.push_back(root);

        std::cerr << "Root node " << GLMUtils::vectorToString(walkPoint) << "is movable, AI walk grid generation starts." << std::endl;
    } else {
        std::cerr << "Root node " << GLMUtils::vectorToString(walkPoint) << "is not movable, AI walk grid generation failed. Please check map." << std::endl;
        return root;
    }
    size_t indexOfFoundNode;
    std::shared_ptr<AIMovementNode> current;
    while (!frontier.empty()) {
        current = frontier.front();
        frontier.pop();

        if(doneNodes.size() %10000 == 0) {
            std::cout << "After "<< SDL_GetTicks() << " current done " << doneNodes.size() << " nodes, partial " << visited.size() << " last node: " << glm::to_string(current->getPosition()) << std::endl;
        }

        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                if (i == 0 && j == 0) {
                    continue; //skip the center, it is the self
                } else {
                    int neighbourIndex = (i + 1) * 3 + (j + 1);
                    if(current->getNeighbour(neighbourIndex) != nullptr) {
                        continue;//already set
                    }
                    isMovable = true;
                    glm::vec3 neighbourPosition = current->getPosition() + glm::vec3(i, 0, j);
                    if (!setProperHeight(&neighbourPosition, floatingHeight, floatingHeight + 1.0f, staticWorld)) {
                        isMovable = false;
                    }

                    if (neighbourPosition.x < min.x || neighbourPosition.y < min.y || neighbourPosition.z < min.z ||
                        neighbourPosition.x > max.x || neighbourPosition.y > max.y || neighbourPosition.z > max.z) {
                        //this means this position is out of whole world AABB, skip
                        continue;
                    }

                    std::shared_ptr<AIMovementNode> visitedNode = isAlreadyVisited(neighbourPosition, indexOfFoundNode);
                    if (visitedNode != nullptr) {
                        //std::cout << "already visited node at " << GLMUtils::vectorToString(neighbourPosition) << std::endl;
                        //std::cout << "already visited node position is " << GLMUtils::vectorToString(visitedNode->getPosition()) << std::endl;
                        current->setNeighbour(neighbourIndex, visitedNode);
                    } else {
                        //std::cout << "adding new node with position " << GLMUtils::vectorToString(neighbourPosition) << std::endl;
                        staticWorld->addCollisionObject(sharedGhostObject, collisionGroup, collisionMask);
                        sharedGhostObject->setWorldTransform(
                                btTransform(btQuaternion::getIdentity(), GLMConverter::GLMToBlt(neighbourPosition)));
                        isMovable = isMovable && !isThereCollision(staticWorld);
                        staticWorld->removeCollisionObject(sharedGhostObject);


                        std::shared_ptr<AIMovementNode> neighbour =std::make_shared<AIMovementNode>(getNextID(), neighbourPosition);
                        neighbour->setIsMovable(isMovable);
                        current->setNeighbour(neighbourIndex, neighbour);
                        visited.push_back(neighbour);
                        if(isMovable) {
                            frontier.push(neighbour);
                        }
                    }
                }
            }
        }
        if(isAlreadyVisited(current->getPosition(), indexOfFoundNode)) {
            visited.erase(visited.begin() + indexOfFoundNode);
        }

        doneNodes.push_back(current);
        if(current->getPosition().x > this->max.x) { this->max.x = current->getPosition().x;}
        if(current->getPosition().y > this->max.y) { this->max.y = current->getPosition().y;}
        if(current->getPosition().z > this->max.z) { this->max.z = current->getPosition().z;}

        if(current->getPosition().x < this->min.x) { this->min.x = current->getPosition().x;}
        if(current->getPosition().y < this->min.y) { this->min.y = current->getPosition().y;}
        if(current->getPosition().z < this->min.z) { this->min.z = current->getPosition().z;}

    }

    std::cout << "Walk grid creation finished with " << doneNodes.size() << " nodes between " << glm::to_string(this->min) << ", " << glm::to_string(this->max) << std::endl;
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
            for (int p = 0; p < manifold->getNumContacts(); ++p) {
                const btManifoldPoint &pt = manifold->getContactPoint(p);

                if (pt.getDistance() < 0.f) {
                    // There is a collision
                    return true;
                }
            }
        }
        sharedManifoldArray.resize(0);

    }
    return false;
}

AIMovementGrid::AIMovementGrid(glm::vec3 startPoint, btDiscreteDynamicsWorld *staticOnlyPhysicsWorld, glm::vec3 min,
                               glm::vec3 max, uint32_t collisionGroup, uint32_t collisionMask) {
    //sharedGhostObject->setCollisionShape(new btBoxShape(btVector3(1.0f,1.0f,1.0f)));
    //sharedGhostObject->setCollisionShape(new btCapsuleShape(1,1));
    ghostShape = new btCapsuleShape(capsuleRadius, capsuleHeight);

    sharedGhostObject->setCollisionShape(ghostShape);
    sharedGhostObject->setCollisionFlags(
            sharedGhostObject->getCollisionFlags());
    sharedGhostObject->setWorldTransform(btTransform(btQuaternion::getIdentity(), GLMConverter::GLMToBlt(startPoint)));
    std::cout << "Start generating AI walk grid" << std::endl;
    doneNodes.push_back(std::make_shared<AIMovementNode>(0, glm::vec3(0,200,0)));//0 index element should be empty
    root = walkMonster(startPoint, staticOnlyPhysicsWorld, min, max, collisionGroup, collisionMask);
    std::cout << "Finished generating AI walk grid, created " << visited.size() << " nodes, checked for collision "
              << isThereCollisionCounter << " times." << std::endl;
    staticOnlyPhysicsWorld->removeCollisionObject(sharedGhostObject);
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

    long start = SDL_GetTicks();

    fromAINode = aStarPath(root, from, 0, route);
    if (fromAINode == nullptr) {
        return false;
    }

    aStarPath(fromAINode, to, maximumNumberOfNodes, route);
    long end = SDL_GetTicks();
    std::cout << "route set " << end - start << std::endl;



    return true;
}

void AIMovementGrid::debugDraw(BulletDebugDrawer *debugDrawer) const {
    glm::vec3 toColor, fromColor;
    for (size_t i = 1; i < doneNodes.size(); i++) {
        int neighbourCount = 5; //if not an edge, just draw first 4, the last 4 should be rendered by the neighbours
        if (doneNodes[i]->isIsMovable()) {
            fromColor = glm::vec3(1, 1, 1);
        } else {
            fromColor = glm::vec3(1, 0, 0);
            neighbourCount = 9; //if on an edge, neighbours will not be able to draw rest, draw all.
        }
        for (int j = 0; j < neighbourCount; ++j) {
            if (doneNodes[i]->getNeighbour(j) != nullptr) {
                if (doneNodes[i]->getNeighbour(j)->isIsMovable()) {
                    toColor = glm::vec3(1, 1, 1);
                } else {
                    toColor = glm::vec3(1, 0, 0);
                }
                debugDrawer->drawLine(GLMConverter::GLMToBlt(doneNodes[i]->getPosition()),
                                      GLMConverter::GLMToBlt(doneNodes[i]->getNeighbour(j)->getPosition()),
                                      GLMConverter::GLMToBlt(fromColor), GLMConverter::GLMToBlt(toColor));
            }
        }
    }
    //Same with visited nodes
    for (size_t i = 1; i < visited.size(); i++) {
        int neighbourCount = 5; //if not an edge, just draw first 4, the last 4 should be rendered by the neighbours
        if (visited[i]->isIsMovable()) {
            fromColor = glm::vec3(1, 1, 1);
        } else {
            fromColor = glm::vec3(1, 0, 0);
            neighbourCount = 9; //if on an edge, neighbours will not be able to draw rest, draw all.
        }
        for (int j = 0; j < neighbourCount; ++j) {
            if (visited[i]->getNeighbour(j) != nullptr) {
                if (visited[i]->getNeighbour(j)->isIsMovable()) {
                    toColor = glm::vec3(1, 1, 1);
                } else {
                    toColor = glm::vec3(1, 0, 0);
                }
                debugDrawer->drawLine(GLMConverter::GLMToBlt(visited[i]->getPosition()),
                                      GLMConverter::GLMToBlt(visited[i]->getNeighbour(j)->getPosition()),
                                      GLMConverter::GLMToBlt(fromColor), GLMConverter::GLMToBlt(toColor));
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

    currentElement = aiGridDocument.NewElement("MaximumNodeID");
    currentElement->SetText(nextPossibleIndex);
    rootNode->InsertEndChild(currentElement);

    tinyxml2::XMLElement * nodesElement = aiGridDocument.NewElement("Nodes");

    for (auto nodeIt = doneNodes.begin(); nodeIt != doneNodes.end(); ++nodeIt) {
        serializeNode(aiGridDocument, rootNode, *nodeIt);
    }

    for (auto nodeIt = visited.begin(); nodeIt != visited.end(); ++nodeIt) {
        serializeNode(aiGridDocument, rootNode, *nodeIt);
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
            positionXElement->SetText(std::__cxx11::to_string(nodeToSerialize->getPosition().x).c_str());
            currentElement->InsertEndChild(positionXElement);

            tinyxml2::XMLElement *positionYElement = aiGridDocument.NewElement("Y");
            positionYElement->SetText(std::__cxx11::to_string(nodeToSerialize->getPosition().y).c_str());
            currentElement->InsertEndChild(positionYElement);

            tinyxml2::XMLElement *positionZElement = aiGridDocument.NewElement("Z");
            positionZElement->SetText(std::__cxx11::to_string(nodeToSerialize->getPosition().z).c_str());
            currentElement->InsertEndChild(positionZElement);

        }
    nodeElement->InsertEndChild(currentElement);


    for (int i = 0; i < 9; ++i) {
            if(i == 4) {
                continue;//4 is self
            }
            currentElement = aiGridDocument.NewElement("Nb");//neighbour
            currentElement->SetAttribute("Ps", std::__cxx11::to_string(i).c_str());//position
            if(nodeToSerialize->getNeighbour(i) != nullptr) {
                currentElement->SetText(std::__cxx11::to_string(nodeToSerialize->getNeighbour(i)->getID()).c_str());
            } else {
                currentElement->SetText("0");//writing 0 if null
            }
            nodeElement->InsertEndChild(currentElement);
        }
    rootNode->InsertEndChild(nodeElement);
}

AIMovementGrid *AIMovementGrid::deserialize(const std::string &fileName) {
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = xmlDoc.LoadFile(fileName.c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Error loading XML "<< fileName << ": " <<  xmlDoc.ErrorName() << std::endl;
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
    std::cout << "Reading AI WalkGrid from file:" << fileNameNode->GetText() << std::endl;

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
