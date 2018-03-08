//
// Created by Engin Manap on 13.02.2016.
//


#include "World.h"
#include "AI/HumanEnemy.h"
#include "Players/FreeCursorPlayer.h"

World::World(GLHelper *glHelper, Options *options, const std::string& worldFileName) : options(options), glHelper(glHelper), fontManager(glHelper) {

    physicalPlayer = new PhysicalPlayer(options);
    currentPlayer = physicalPlayer;
    camera = new Camera(options, physicalPlayer);

    assetManager = new AssetManager(glHelper);
    // physics init
    broadphase = new btDbvtBroadphase();
    ghostPairCallback = new btGhostPairCallback();
    broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(
            ghostPairCallback);    // Needed once to enable ghost objects inside Bullet

    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);

    solver = new btSequentialImpulseConstraintSolver;

    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, -10, 0));
    debugDrawer = new BulletDebugDrawer(glHelper, options);
    dynamicsWorld->setDebugDrawer(debugDrawer);
    dynamicsWorld->getDebugDrawer()->setDebugMode(dynamicsWorld->getDebugDrawer()->DBG_NoDebug);
    //dynamicsWorld->getDebugDrawer()->setDebugMode(dynamicsWorld->getDebugDrawer()->DBG_MAX_DEBUG_DRAW_MODE);

    GUILayer *layer1 = new GUILayer(glHelper, debugDrawer, 1);
    layer1->setDebug(false);

    shadowMapProgramDirectional = new GLSLProgram(glHelper, "./Data/Shaders/ShadowMap/vertexDirectional.glsl",
                                                  "./Data/Shaders/ShadowMap/fragmentDirectional.glsl", false);
    shadowMapProgramPoint = new GLSLProgram(glHelper, "./Data/Shaders/ShadowMap/vertexPoint.glsl",
                                            "./Data/Shaders/ShadowMap/geometryPoint.glsl",
                                            "./Data/Shaders/ShadowMap/fragmentPoint.glsl", false);
    shadowMapProgramPoint->setUniform("farPlanePoint", options->getLightPerspectiveProjectionValues().z);

    if(!loadMapFromXML(worldFileName)) {
        exit(-1);
    }

    //adding camera after dynamic world because static only world is needed for ai movement grid generation
    currentPlayer->registerToPhysicalWorld(dynamicsWorld, worldAABBMin, worldAABBMax);

    GUIText *tr = new GUIText(glHelper, fontManager.getFont("Data/Fonts/Wolf_in_the_City_Light.ttf", 64), "Limon Engine",
                              glm::vec3(0, 0, 0));
    //tr->setScale(0.25f,0.25f);
    tr->set2dWorldTransform(glm::vec2(options->getScreenWidth()/2, options->getScreenHeight()-20), 0.0f);
    layer1->addGuiElement(tr);

    tr = new GUIText(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 16), "Version 0.2",
                     glm::vec3(255, 255, 255));
    tr->set2dWorldTransform(glm::vec2(options->getScreenWidth() - 50, 100), -1 * options->PI / 2);
    layer1->addGuiElement(tr);

    cursor = new GUIText(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 16), "+",
                     glm::vec3(255, 255, 255));
    cursor->set2dWorldTransform(glm::vec2(options->getScreenWidth()/2.0f, options->getScreenHeight()/2.0f), -1 * options->PI / 4);
    layer1->addGuiElement(cursor);

    trd = new GUITextDynamic(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 16), glm::vec3(0, 0, 0), 640, 380, options);
    trd->set2dWorldTransform(glm::vec2(320, options->getScreenHeight()-200), 0.0f);
    layer1->addGuiElement(trd);



    tr = new GUIFPSCounter(glHelper, fontManager.getFont("Data/Fonts/Helvetica-Normal.ttf", 16), "0",
                           glm::vec3(204, 204, 0));
    tr->set2dWorldTransform(glm::vec2(options->getScreenWidth() - 50, options->getScreenHeight() - 18), 0);
    layer1->addGuiElement(tr);

    guiLayers.push_back(layer1);

    //Since all Lights are static, and we use UBOs for light values, we can set them here, and not update
    for (unsigned int i = 0; i < lights.size(); ++i) {
        glHelper->setLight(*(lights[i]), i);
    }
}

 bool World::checkPlayerVisibility(const glm::vec3 &from, const std::string &fromName) {
     //FIXME this debug draw creates a flicker, because we redraw frames that surpass 60. we need duration for debug draw to prevent it
     //debugDrawer->drawLine(GLMConverter::GLMToBlt(from), camera.getRigidBody()->getCenterOfMassPosition(), btVector3(1,0,0));
     btCollisionWorld::AllHitsRayResultCallback RayCallback(GLMConverter::GLMToBlt(from), GLMConverter::GLMToBlt(currentPlayer->getPosition()));

     dynamicsWorld->rayTest(
             GLMConverter::GLMToBlt(from),
             GLMConverter::GLMToBlt(currentPlayer->getPosition()),
             RayCallback
     );

     //debugDrawer->flushDraws();
     if (RayCallback.hasHit()) {
         for (int i = 0; i < RayCallback.m_collisionObjects.size(); ++i) {
             std::string *objectType = (std::string *) RayCallback.m_collisionObjects[i]->getUserPointer();
             if (*(objectType) != "player" &&
                 *(objectType) != fromName) {
                 return false;
             }
         }
         return true;
     }
     return false;//if ray did not hit anything, return false. This should never happen
 }

void World::play(Uint32 simulationTimeFrame, InputHandler &inputHandler) {
    //every time we call this method, we increase the time only by simulationTimeframe
    gameTime += simulationTimeFrame;
    dynamicsWorld->stepSimulation(simulationTimeFrame / 1000.0f);
    currentPlayer->processPhysicsWorld(dynamicsWorld);

    for (unsigned int i = 0; i < objects.size(); ++i) {
        objects[i]->setupForTime(gameTime);
        objects[i]->updateTransformFromPhysics();
    }

    for (unsigned int i = 0; i < guiLayers.size(); ++i) {
        guiLayers[i]->setupForTime(gameTime);
    }

    for (unsigned int j = 0; j < actors.size(); ++j) {
        ActorInformation information = fillActorInformation(j);
        actors[j]->play(gameTime,information, options);
    }

    //end of physics step

    handlePlayerInput(inputHandler);
}

ActorInformation World::fillActorInformation(int j) {
    ActorInformation information;
    //FIXME this is just for test
    information.canSeePlayerDirectly = checkPlayerVisibility(actors[j]->getPosition(), "./Data/Models/ArmyPilot/ArmyPilot.mesh.xml");
    glm::vec3 front = actors[j]->getFrontVector();
    glm::vec3 rayDir = currentPlayer->getPosition() - actors[j]->getPosition();
    float cosBetween = glm::dot(normalize(front), normalize(rayDir));
    information.cosineBetweenPlayer = cosBetween;
    information.playerDirection = normalize(rayDir);
    if(cosBetween > 0) {
            information.isPlayerFront = true;
            information.isPlayerBack = false;
        } else {
            information.isPlayerFront = false;
            information.isPlayerBack = true;
        }
    //now we know if it is front or back. we can check up, down, left, right
    //remove the y component, and test for left, right
    glm::vec3 rayDirWithoutY = rayDir;
    rayDirWithoutY.y = 0;
    glm::vec3 frontWithoutY = front;
    frontWithoutY.y = 0;
    glm::vec3 crossBetween = cross(normalize(frontWithoutY), normalize(rayDirWithoutY));
    float cosineForSide = glm::dot(normalize(frontWithoutY), normalize(rayDirWithoutY));
    information.cosineBetweenPlayerForSide = cosineForSide;
    if(crossBetween.y > 0){
            information.isPlayerRight = false;
            information.isPlayerLeft = true;
        } else {
            information.isPlayerRight = true;
            information.isPlayerLeft = false;
        }
    //now we need up and down. For that, normally we can remove z or x, but since camera is z alone at start, I will use x
    rayDir.x = 0;
    front.x = 0;
    crossBetween = cross(normalize(front), normalize(rayDir));
    if(crossBetween.x > 0){
            information.isPlayerUp = false;
            information.isPlayerDown = true;
        } else {
            information.isPlayerUp = true;
            information.isPlayerDown = false;
        }
    std::vector<glm::vec3> route;
    glm::vec3 playerPosWithGrid = currentPlayer->getPosition();
    bool isPlayerReachable = grid->setProperHeight(&playerPosWithGrid, AIMovementGrid::floatingHeight, 0.0f, dynamicsWorld);
    if(isPlayerReachable && grid->coursePath(actors[j]->getPosition() + glm::vec3(0, AIMovementGrid::floatingHeight, 0), playerPosWithGrid, j, &route)) {
        if (route.empty()) {
            information.toPlayerRoute = glm::vec3(0, 0, 0);
            information.canGoToPlayer = false;
        } else {
            //Normally, this information should be used for straightening the path, but not yet.
            information.toPlayerRoute = route[route.size() - 1] - actors[j]->getPosition() - glm::vec3(0, 2.0f, 0);
            information.canGoToPlayer = true;
        }
    }
    return information;
}

void World::handlePlayerInput(InputHandler &inputHandler) {
    float xLook, yLook;
    if (inputHandler.getMouseChange(xLook, yLook)) {
        currentPlayer->rotate(xLook, yLook);
    }

    if (inputHandler.getInputEvents(inputHandler.RUN)) {
        if(inputHandler.getInputStatus(inputHandler.RUN)) {
            options->setMoveSpeed(Options::RUN);
        } else {
            options->setMoveSpeed(Options::WALK);
        }
    }

    if(inputHandler.getInputEvents(inputHandler.MOUSE_BUTTON_LEFT)) {
        if(inputHandler.getInputStatus(inputHandler.MOUSE_BUTTON_LEFT)) {
            std::string *objectName = (std::string *) getPointedObject();
            std::string logLine;
            if (objectName != nullptr) {
                logLine = "object to pick is " + *objectName;
            } else {
                logLine = "no object to pick.";
            }
            options->getLogger()->log(Logger::INPUT, Logger::DEBUG, logLine);
        }
    }

    PhysicalPlayer::moveDirections direction = PhysicalPlayer::NONE;
    //ignore if both are pressed.
    if (inputHandler.getInputStatus(inputHandler.MOVE_FORWARD) !=
        inputHandler.getInputStatus(inputHandler.MOVE_BACKWARD)) {
        if (inputHandler.getInputStatus(inputHandler.MOVE_FORWARD)) {
            direction = Player::FORWARD;
        } else {
            direction = Player::BACKWARD;
        }
    }
    if (inputHandler.getInputStatus(inputHandler.MOVE_LEFT) != inputHandler.getInputStatus(inputHandler.MOVE_RIGHT)) {
        if (inputHandler.getInputStatus(inputHandler.MOVE_LEFT)) {
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
    if (inputHandler.getInputStatus(inputHandler.JUMP) && inputHandler.getInputEvents(inputHandler.JUMP)) {
        direction = Player::UP;
    }
    if (inputHandler.getInputEvents(inputHandler.DEBUG) && inputHandler.getInputStatus(inputHandler.DEBUG)) {
        if(this->dynamicsWorld->getDebugDrawer()->getDebugMode() == btIDebugDraw::DBG_NoDebug) {
            this->dynamicsWorld->getDebugDrawer()->setDebugMode(
                    this->dynamicsWorld->getDebugDrawer()->DBG_MAX_DEBUG_DRAW_MODE | this->dynamicsWorld->getDebugDrawer()->DBG_DrawAabb | this->dynamicsWorld->getDebugDrawer()->DBG_DrawConstraints | this->dynamicsWorld->getDebugDrawer()->DBG_DrawConstraintLimits);
            this->options->getLogger()->log(Logger::INPUT, Logger::INFO, "Debug enabled");
            this->guiLayers[0]->setDebug(true);
            //switch control to debug player
            if(debugPlayer == nullptr) {
                debugPlayer = new FreeCursorPlayer(options, cursor);
                debugPlayer->registerToPhysicalWorld(dynamicsWorld, worldAABBMin, worldAABBMax);
            }
            debugPlayer->setPositionAndRotation(currentPlayer->getPosition(), currentPlayer->getLookDirection());
            currentPlayer = debugPlayer;
            camera->setCameraAttachment(debugPlayer);
        } else {
            this->dynamicsWorld->getDebugDrawer()->setDebugMode(this->dynamicsWorld->getDebugDrawer()->DBG_NoDebug);
            this->options->getLogger()->log(Logger::INPUT, Logger::INFO, "Debug disabled");
            this->guiLayers[0]->setDebug(false);
            physicalPlayer->setPositionAndRotation(currentPlayer->getPosition(), currentPlayer->getLookDirection());
            currentPlayer = physicalPlayer;
            camera->setCameraAttachment(physicalPlayer);
            dynamicsWorld->updateAabbs();//FIXME So this is wrong
        }
    }
    //if none, camera should handle how to get slower.
    currentPlayer->move(direction);

}

void *World::getPointedObject() const {
    glm::vec3 from, lookDirection;
    currentPlayer->getWhereCameraLooks(from, lookDirection);
    //we want to extend to vector to world AABB limit
    float maxFactor = 0;

    if(lookDirection.x > 0 ) {
        //so we are looking at positive x. determine how many times the ray x we need
        maxFactor = (worldAABBMax.x - from.x) / lookDirection.x;
    } else {
        maxFactor = (worldAABBMin.x - from.x) / lookDirection.x; //Mathematically this should be (from - world.min) / -1 * lookdir, but it cancels out
    }

    if(lookDirection.y > 0 ) {
        std::max(maxFactor, (worldAABBMax.y - from.y) / lookDirection.y);
    } else {
        std::max(maxFactor, (worldAABBMin.y - from.y) / lookDirection.y);//Mathematically this should be (from - world.min) / -1 * lookdir, but it cancels out
    }

    if(lookDirection.z > 0 ) {
        std::max(maxFactor, (worldAABBMax.z - from.z) / lookDirection.z);
    } else {
        std::max(maxFactor, (worldAABBMin.z - from.z) / lookDirection.z);//Mathematically this should be (from - world.min) / -1 * lookdir, but it cancels out
    }
    lookDirection = lookDirection * maxFactor;
    glm::vec3 to = lookDirection + from;
    btCollisionWorld::ClosestRayResultCallback RayCallback(GLMConverter::GLMToBlt(from), GLMConverter::GLMToBlt(to));

    dynamicsWorld->rayTest(
                GLMConverter::GLMToBlt(from),
                GLMConverter::GLMToBlt(to),
                RayCallback
        );

    //debugDrawer->flushDraws();
    if (RayCallback.hasHit()) {
        return RayCallback.m_collisionObject->getUserPointer();
    } else {
        return nullptr;
    }
}

void World::render() {
    for (unsigned int i = 0; i < lights.size(); ++i) {
        if(lights[i]->getLightType() != Light::DIRECTIONAL) {
            continue;
        }
        //generate shadow map
        glHelper->switchRenderToShadowMapDirectional(i);
        shadowMapProgramDirectional->setUniform("renderLightIndex", (int)i);
        for (std::vector<PhysicalRenderable *>::iterator it = objects.begin(); it != objects.end(); ++it) {
            (*it)->renderWithProgram(*shadowMapProgramDirectional);
        }
    }

    for (unsigned int i = 0; i < lights.size(); ++i) {
        if(lights[i]->getLightType() != Light::POINT) {
            continue;
        }
        //generate shadow map
        glHelper->switchRenderToShadowMapPoint();
        //FIXME why are these set here?
        shadowMapProgramPoint->setUniform("renderLightIndex", (int)i);
        //FIXME this is suppose to be an option //FarPlanePoint is set at declaration, since it is a constant
        shadowMapProgramPoint->setUniform("farPlanePoint", 100.0f);
        for (std::vector<PhysicalRenderable *>::iterator it = objects.begin(); it != objects.end(); ++it) {
            (*it)->renderWithProgram(*shadowMapProgramPoint);
        }
    }

    glHelper->switchRenderToDefault();
    if(sky!=nullptr) {
        sky->render();//this is moved to the top, because transparency can create issues if this is at the end
    }

    if(camera->isDirty()) {
        glHelper->setPlayerMatrices(camera->getPosition(), camera->getCameraMatrix());
    }


    for (std::vector<PhysicalRenderable *>::iterator it = objects.begin(); it != objects.end(); ++it) {
        (*it)->render();
    }

    dynamicsWorld->debugDrawWorld();
    if (this->dynamicsWorld->getDebugDrawer()->getDebugMode() != btIDebugDraw::DBG_NoDebug) {
        debugDrawer->drawLine(btVector3(0, 0, 0), btVector3(0, 250, 0), btVector3(1, 1, 1));
        //draw the ai-grid
        grid->debugDraw(debugDrawer);
    }

    debugDrawer->flushDraws();


    //since gui uses blending, everything must be already rendered.
    // Also, since gui elements only depth test each other, clear depth buffer
    glHelper->clearDepthBuffer();

    for (std::vector<GUILayer *>::iterator it = guiLayers.begin(); it != guiLayers.end(); ++it) {
        (*it)->render();
    }


}

World::~World() {
    delete dynamicsWorld;

    //FIXME clear GUIlayer elements
    for (std::vector<PhysicalRenderable *>::iterator it = objects.begin(); it != objects.end(); ++it) {
        delete (*it);
    }
    delete sky;

    for (std::vector<Light *>::iterator it = lights.begin(); it != lights.end(); ++it) {
        delete (*it);
    }
    delete debugDrawer;
    delete solver;
    delete collisionConfiguration;
    delete dispatcher;
    delete broadphase;
    delete ghostPairCallback;

    delete grid;
    delete camera;
    delete physicalPlayer;
    if(debugPlayer!= nullptr) {
        delete debugPlayer;
    }

}

bool World::loadMapFromXML(const std::string& worldFileName) {

    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = xmlDoc.LoadFile(worldFileName.c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Error loading XML: " <<  xmlDoc.ErrorName() << std::endl;
        exit(-1);
    }

    tinyxml2::XMLNode * worldNode = xmlDoc.FirstChild();
    if (worldNode == nullptr) {
        std::cerr << "World xml is not a valid XML." << std::endl;
        return false;
    }

    tinyxml2::XMLElement* worldName =  worldNode->FirstChildElement("Name");
    if (worldName == nullptr) {
        std::cerr << "World must have a name." << std::endl;
        return false;
    }
    std::cout << "read name as " << worldName->GetText() << std::endl;

    //load objects
    if(!loadObjectsFromXML(worldNode)) {
        return false;
    }
    //load Skymap
    loadSkymap(worldNode);

    //load lights
    loadLights(worldNode);

    return true;
}

bool World::loadObjectsFromXML(tinyxml2::XMLNode *worldNode) {
    tinyxml2::XMLElement* objectsListNode =  worldNode->FirstChildElement("Objects");
    if (objectsListNode == nullptr) {
        std::cerr << "World Must have and Objects clause." << std::endl;
        return false;
    }

    tinyxml2::XMLElement* objectNode =  objectsListNode->FirstChildElement("Object");
    if (objectNode == nullptr) {
        std::cerr << "World Must have at least one object." << std::endl;
        return false;
    }
    Model *xmlModel;
    tinyxml2::XMLElement* objectAttribute;
    tinyxml2::XMLElement* objectAttributeAttribute;
    std::string modelFile;
    float modelMass;
    float x,y,z,w;
    std::vector<Model*> notStaticObjects;
    bool isAIGridStartPointSet = false;
    glm::vec3 aiGridStartPoint;
    while(objectNode != nullptr) {
        objectAttribute =  objectNode->FirstChildElement("File");
        if (objectAttribute == nullptr) {
            std::cerr << "Object must have a source file." << std::endl;
            return false;
        }
        modelFile = objectAttribute->GetText();
        objectAttribute =  objectNode->FirstChildElement("Mass");
        if (objectAttribute == nullptr) {
            std::cout << "Object does not have mass, assume 0." << std::endl;
            modelMass = 0;
        } else {
            modelMass = std::stof(objectAttribute->GetText());
        }

        xmlModel = new Model(assetManager,modelMass, modelFile);

        objectAttribute =  objectNode->FirstChildElement("Scale");
        if (objectAttribute == nullptr) {
            std::cout << "Object does not have scale." << std::endl;
        } else {
            objectAttributeAttribute =  objectAttribute->FirstChildElement("X");
            if(objectAttributeAttribute != nullptr) {
                x = std::stof(objectAttributeAttribute->GetText());
            } else {
                x = 1.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Y");
            if(objectAttributeAttribute != nullptr) {
                y = std::stof(objectAttributeAttribute->GetText());
            } else {
                y = 1.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Z");
            if(objectAttributeAttribute != nullptr) {
                z = std::stof(objectAttributeAttribute->GetText());
            } else {
                z = 1.0;
            }
            xmlModel->addScale(glm::vec3(x,y,z));
        }

        objectAttribute =  objectNode->FirstChildElement("Translate");
        if (objectAttribute == nullptr) {
            std::cout << "Object does not have translate." << std::endl;
        } else {
            objectAttributeAttribute =  objectAttribute->FirstChildElement("X");
            if(objectAttributeAttribute != nullptr) {
                x = std::stof(objectAttributeAttribute->GetText());
            } else {
                x = 0.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Y");
            if(objectAttributeAttribute != nullptr) {
                y = std::stof(objectAttributeAttribute->GetText());
            } else {
                y = 0.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Z");
            if(objectAttributeAttribute != nullptr) {
                z = std::stof(objectAttributeAttribute->GetText());
            } else {
                z = 0.0;
            }
            xmlModel->addTranslate(glm::vec3(x,y,z));
        }

        objectAttribute =  objectNode->FirstChildElement("Rotate");
        if (objectAttribute == nullptr) {
            std::cout << "Object does not have translate." << std::endl;
        } else {
            objectAttributeAttribute =  objectAttribute->FirstChildElement("X");
            if(objectAttributeAttribute != nullptr) {
                x = std::stof(objectAttributeAttribute->GetText());
            } else {
                x = 0.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Y");
            if(objectAttributeAttribute != nullptr) {
                y = std::stof(objectAttributeAttribute->GetText());
            } else {
                y = 0.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("Z");
            if(objectAttributeAttribute != nullptr) {
                z = std::stof(objectAttributeAttribute->GetText());
            } else {
                z = 0.0;
            }
            objectAttributeAttribute =  objectAttribute->FirstChildElement("W");
            if(objectAttributeAttribute != nullptr) {
                w = std::stof(objectAttributeAttribute->GetText());
            } else {
                w = 0.0;
            }
            xmlModel->addOrientation(glm::quat(w, x, y, z));
        }
        //Since we are not loading objects recursively, these can be set here safely
        objectAttribute =  objectNode->FirstChildElement("AI");
        if (objectAttribute == nullptr) {
            std::cout << "Object does not have AI." << std::endl;
        } else {
            if (!isAIGridStartPointSet) {
                aiGridStartPoint = GLMConverter::BltToGLM(xmlModel->getRigidBody()->getCenterOfMassPosition()) +
                                   glm::vec3(0, 2.0f, 0);
                isAIGridStartPointSet = true;
            }
            std::cout << "Object has AI." << std::endl;
            HumanEnemy* newEnemy = new HumanEnemy();
            newEnemy->setModel(xmlModel);
            this->actors.push_back(newEnemy);
        }

        objectAttribute =  objectNode->FirstChildElement("AI");
        if (objectAttribute == nullptr) {
            std::cout << "Object does not have AI." << std::endl;
        } else {
            if (!isAIGridStartPointSet) {
                aiGridStartPoint = GLMConverter::BltToGLM(xmlModel->getRigidBody()->getCenterOfMassPosition()) +
                                   glm::vec3(0, 2.0f, 0);
                isAIGridStartPointSet = true;
            }
            std::cout << "Object has AI." << std::endl;
            HumanEnemy* newEnemy = new HumanEnemy();
            newEnemy->setModel(xmlModel);
            this->actors.push_back(newEnemy);
        }

        objectAttribute =  objectNode->FirstChildElement("Animation");
        if (objectAttribute == nullptr) {
            std::cout << "Object does not have default animation." << std::endl;
        } else {
            xmlModel->setAnimation(objectAttribute->GetText());
        }

        //ADD NEW ATTRIBUTES GOES UP FROM HERE
        // We will add static objects first, build AI grid, then add other objects
        if(xmlModel->getMass() == 0 && !xmlModel->isAnimated()) {
            addModelToWorld(xmlModel);
        } else {
            notStaticObjects.push_back(xmlModel);
        }
        objectNode = objectNode->NextSiblingElement("Object");
    } // end of while (objects)

    grid = new AIMovementGrid(aiGridStartPoint, dynamicsWorld, worldAABBMin, worldAABBMax);

    for (unsigned int i = 0; i < notStaticObjects.size(); ++i) {
        addModelToWorld(notStaticObjects[i]);
    }
    return true;
}

void World::addModelToWorld(Model *xmlModel) {
    xmlModel->getWorldTransform();
    objects.push_back(xmlModel);
    rigidBodies.push_back(xmlModel->getRigidBody());
    dynamicsWorld->addRigidBody(xmlModel->getRigidBody());
    btVector3 aabbMin, aabbMax;
    xmlModel->getRigidBody()->getAabb(aabbMin, aabbMax);
    updateWorldAABB(GLMConverter::BltToGLM(aabbMin), GLMConverter::BltToGLM(aabbMax));
}

bool World::loadSkymap(tinyxml2::XMLNode *worldNode) {
    tinyxml2::XMLElement* skyNode =  worldNode->FirstChildElement("Sky");
    if (skyNode == nullptr) {
        std::cerr << "Sky clause not found." << std::endl;
        return false;
    }

    tinyxml2::XMLElement* imagesPath =  skyNode->FirstChildElement("ImagesPath");
    if (imagesPath == nullptr) {
        std::cerr << "Sky map must have the root path." << std::endl;
        return false;
    }

    std::string path, left, right, top, bottom, front, back;
    path = imagesPath->GetText();

    tinyxml2::XMLElement* leftNode =  skyNode->FirstChildElement("Left");
    if (leftNode == nullptr) {
        std::cerr << "Sky map must have left image name." << std::endl;
        return false;
    }
    left = leftNode->GetText();

    tinyxml2::XMLElement* rightNode =  skyNode->FirstChildElement("Right");
    if (rightNode == nullptr) {
        std::cerr << "Sky map must have right image name." << std::endl;
        return false;
    }
    right = rightNode->GetText();

    tinyxml2::XMLElement* topNode =  skyNode->FirstChildElement("Top");
    if (topNode == nullptr) {
        std::cerr << "Sky map must have top image name." << std::endl;
        return false;
    }
    top = topNode->GetText();

    tinyxml2::XMLElement* bottomNode =  skyNode->FirstChildElement("Bottom");
    if (bottomNode == nullptr) {
        std::cerr << "Sky map must have bottom image name." << std::endl;
        return false;
    }
    bottom = bottomNode->GetText();

    tinyxml2::XMLElement* backNode =  skyNode->FirstChildElement("Back");
    if (backNode == nullptr) {
        std::cerr << "Sky map must have back image name." << std::endl;
        return false;
    }
    back = backNode->GetText();

    tinyxml2::XMLElement* frontNode =  skyNode->FirstChildElement("Front");
    if (frontNode == nullptr) {
        std::cerr << "Sky map must have front image name." << std::endl;
        return false;
    }
    front = frontNode->GetText();

    sky = new SkyBox(assetManager,
                     std::string(path),
                     std::string(right),
                     std::string(left),
                     std::string(top),
                     std::string(bottom),
                     std::string(back),
                     std::string(front)
    );
    return true;
}

bool World::loadLights(tinyxml2::XMLNode *worldNode) {
    tinyxml2::XMLElement* lightsListNode =  worldNode->FirstChildElement("Lights");
    if (lightsListNode == nullptr) {
        std::cerr << "Lights clause not found." << std::endl;
        return false;
    }


    tinyxml2::XMLElement* lightNode =  lightsListNode->FirstChildElement("Light");
    if (lightNode == nullptr) {
        std::cerr << "Lights did not have at least one light." << std::endl;
        return false;
    }

    Light::LightTypes type;
    glm::vec3 position;
    glm::vec3 color;
    Light *xmlLight;
    tinyxml2::XMLElement* lightAttribute;
    tinyxml2::XMLElement* lightAttributeAttribute;
    float x,y,z;
    while(lightNode != nullptr) {
        lightAttribute = lightNode->FirstChildElement("Type");
        if (lightAttribute == nullptr) {
            std::cerr << "Light must have a type." << std::endl;
            return false;
        }

        std::string typeString = lightAttribute->GetText();
        if (typeString == "POINT") {
            type = Light::POINT;
        } else if (typeString == "DIRECTIONAL") {
            type = Light::DIRECTIONAL;
        } else {
            std::cerr << "Light type is not POINT or DIRECTIONAL. it is " << lightAttribute->GetText() << std::endl;
            return false;
        }
        lightAttribute = lightNode->FirstChildElement("Position");
        if (lightAttribute == nullptr) {
            std::cerr << "Light must have a position/direction." << std::endl;
            return false;
        } else {
            lightAttributeAttribute = lightAttribute->FirstChildElement("X");
            if (lightAttributeAttribute != nullptr) {
                x = std::stof(lightAttributeAttribute->GetText());
            } else {
                std::cerr << "Light position/direction missing x." << std::endl;
                return false;
            }
            lightAttributeAttribute = lightAttribute->FirstChildElement("Y");
            if (lightAttributeAttribute != nullptr) {
                y = std::stof(lightAttributeAttribute->GetText());
            } else {
                std::cerr << "Light position/direction missing y." << std::endl;
                return false;
            }
            lightAttributeAttribute = lightAttribute->FirstChildElement("Z");
            if (lightAttributeAttribute != nullptr) {
                z = std::stof(lightAttributeAttribute->GetText());
            } else {
                std::cerr << "Light position/direction missing z." << std::endl;
                return false;
            }
        }
        position.x = x;
        position.y = y;
        position.z = z;

        lightAttribute = lightNode->FirstChildElement("Color");
        if (lightAttribute == nullptr) {
            x = y = z = 1.0f;
        } else {
            lightAttributeAttribute = lightAttribute->FirstChildElement("X");
            if (lightAttributeAttribute != nullptr) {
                x = std::stof(lightAttributeAttribute->GetText());
            } else {
                x = 1.0f;
            }
            lightAttributeAttribute = lightAttribute->FirstChildElement("Y");
            if (lightAttributeAttribute != nullptr) {
                y = std::stof(lightAttributeAttribute->GetText());
            } else {
                y = 1.0f;
            }
            lightAttributeAttribute = lightAttribute->FirstChildElement("Z");
            if (lightAttributeAttribute != nullptr) {
                z = std::stof(lightAttributeAttribute->GetText());
            } else {
                z = 1.0f;
            }
        }
        color.x = x;
        color.y = y;
        color.z = z;

        xmlLight = new Light(type,position,color);
        lights.push_back(xmlLight);
        lightNode =  lightNode->NextSiblingElement("Light");
    }
    return true;
}

void World::updateWorldAABB(glm::vec3 aabbMin, glm::vec3 aabbMax) {
    worldAABBMin = glm::vec3(std::min(aabbMin.x, worldAABBMin.x), std::min(aabbMin.y, worldAABBMin.y), std::min(aabbMin.z, worldAABBMin.z));
    worldAABBMax = glm::vec3(std::max(aabbMax.x, worldAABBMax.x), std::max(aabbMax.y, worldAABBMax.y), std::max(aabbMax.z, worldAABBMax.z));
}
